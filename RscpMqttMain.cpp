#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include "RscpProtocol.h"
#include "RscpTags.h"
#include "RscpMqttMapping.h"
#include "RscpMqttConfig.h"
#include "SocketConnection.h"
#include "AES.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <mosquitto.h>
#include <thread>
#include <regex>
#include <mutex>

#define AES_KEY_SIZE            32
#define AES_BLOCK_SIZE          32

#define CONFIG_FILE             ".config"
#define DEFAULT_INTERVAL_MIN    1
#define DEFAULT_INTERVAL_MAX    10
#define SUBSCRIBE_TOPIC         "e3dc/set/#"

static int iSocket = -1;
static int iAuthenticated = 0;
static AES aesEncrypter;
static AES aesDecrypter;
static uint8_t ucEncryptionIV[AES_BLOCK_SIZE];
static uint8_t ucDecryptionIV[AES_BLOCK_SIZE];

static struct mosquitto *mosq = NULL;
static time_t e3dc_ts = 0;
static time_t e3dc_diff = 0;
static config_t cfg;
static int yesterday = 0;
static int lhour = -1;

std::mutex mtx;

static bool mqttRcvd = false;

int storeMQTTReceivedValue(std::vector<RSCP_MQTT::rec_cache_t> & c, char *topic, char *payload) {
    mtx.lock();
    mqttRcvd = true;
    for (std::vector<RSCP_MQTT::rec_cache_t>::iterator it = c.begin(); it != c.end(); ++it) {
        if (std::regex_match(topic, std::regex(it->topic))) {
            if (!cfg.daemon) printf("MQTT: received topic >%s< payload >%s<\n", it->topic, payload);
            if (std::regex_match(payload, std::regex(it->regex_true))) {
                if (strcmp(it->value_true, "")) strncpy(it->payload, it->value_true, PAYLOAD_SIZE);
                else strncpy(it->payload, payload, PAYLOAD_SIZE);
                it->done = false;
                break;
            } else if (std::regex_match(payload, std::regex(it->regex_false))) {
                if (strcmp(it->value_false, "")) strncpy(it->payload, it->value_false, PAYLOAD_SIZE);
                else strncpy(it->payload, payload, PAYLOAD_SIZE);
                it->done = false;
                break;
            } else {
                it->done = true;
                break;
            }
        }
    }
    mtx.unlock();
    return(0);
}

void mqttCallbackOnConnect(struct mosquitto *mosq, void *obj, int result) {
    if (!result) {
        if (!cfg.daemon) printf("MQTT: subscribing topic >%s<\n", SUBSCRIBE_TOPIC);
        mosquitto_subscribe(mosq, NULL, SUBSCRIBE_TOPIC, cfg.mqtt_qos);
    } else {
        if (!cfg.daemon) printf("MQTT: subscribing topic >%s< failed\n", SUBSCRIBE_TOPIC);
    }
}

void mqttCallbackOnMessage(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    if (mosq && msg && msg->topic && msg->payload) {
        //printf("MQTT: message topic >%s< payload >%s< received\n", msg->topic, (char *)msg->payload);
        storeMQTTReceivedValue(RSCP_MQTT::RscpMqttReceiveCache, msg->topic, (char *)msg->payload);
    }
}

void mqttListener(struct mosquitto *m) {
    if (m) {
         if (!cfg.daemon) printf("MQTT: starting listener loop\n");
         mosquitto_loop_forever(m, -1, 1);
    }
}

int handleMQTT(std::vector<RSCP_MQTT::cache_t> & v, int qos, bool retain) {
    int rc = 0;

    for (std::vector<RSCP_MQTT::cache_t>::iterator it = v.begin(); it != v.end(); ++it) {
        //printf("MQTT: vector topic >%s< payload >%s< %d\n", it->topic, it->payload, it->publish);
        if (it->publish && strcmp(it->topic, "") && strcmp(it->payload, "")) {
            if (!cfg.daemon) printf("MQTT: publish topic >%s< payload >%s<\n", it->topic, it->payload);
            rc = mosquitto_publish(mosq, NULL, it->topic, strlen(it->payload), it->payload, qos, retain);
            it->publish = false;
        }
    }
    return(rc);
}

int logCache(std::vector<RSCP_MQTT::cache_t> & v, char *file, char *prefix) {
    FILE *fp;
    fp = fopen(file, "a");
    if (!fp) return(0);
    for (std::vector<RSCP_MQTT::cache_t>::iterator it = v.begin(); it != v.end(); ++it) {
        if (!cfg.daemon) printf("%s: topic >%s< payload >%s<\n", prefix, it->topic, it->payload);
        fprintf(fp, "%s: topic >%s< payload >%s<\n", prefix, it->topic, it->payload);
    }
    fclose(fp);
    return(1);
}

int refreshCache(std::vector<RSCP_MQTT::cache_t> & v) {
    for (std::vector<RSCP_MQTT::cache_t>::iterator it = v.begin(); it != v.end(); ++it) {
        if (strcmp(it->payload, "")) it->publish = true;
    }
    return(1);
}

int storeResponseValue(std::vector<RSCP_MQTT::cache_t> & c, RscpProtocol *protocol, SRscpValue *response, int day) {
    char buf[PAYLOAD_SIZE];
    int rc = 0;

    for (std::vector<RSCP_MQTT::cache_t>::iterator it = c.begin(); it != c.end(); ++it) {
        if ((it->day == day) && (it->tag == response->tag)) {
            switch (it->type) {
                case RSCP::eTypeBool: {
                    if (protocol->getValueAsBool(response)) strcpy(buf, "true");
                    else strcpy(buf, "false");
                    if (strcmp(it->payload, buf) || it->day) {
                        strcpy(it->payload, buf);
                        it->publish = true;
                    }
                    break;
                }
                case RSCP::eTypeInt32: {
                    snprintf(buf, PAYLOAD_SIZE, it->fstring, protocol->getValueAsInt32(response));
                    if (strcmp(it->payload, buf) || it->day) {
                        strcpy(it->payload, buf);
                        it->publish = true;
                    }
                    break;
                }
                case RSCP::eTypeUInt32: {
                    snprintf(buf, PAYLOAD_SIZE, it->fstring, protocol->getValueAsUInt32(response));
                    if (strcmp(it->payload, buf) || it->day) {
                        strcpy(it->payload, buf);
                        it->publish = true;
                    }
                    break;
                }
                case RSCP::eTypeChar8: {
                    snprintf(buf, PAYLOAD_SIZE, it->fstring, protocol->getValueAsChar8(response));
                    if (strcmp(it->payload, buf) || it->day) {
                        strcpy(it->payload, buf);
                        it->publish = true;
                    }
                    break;
                }
                case RSCP::eTypeUChar8: {
                    snprintf(buf, PAYLOAD_SIZE, it->fstring, protocol->getValueAsUChar8(response));
                    if (strcmp(it->payload, buf) || it->day) {
                        strcpy(it->payload, buf);
                        it->publish = true;
                    }
                    break;
                }
                case RSCP::eTypeFloat32: {
                    snprintf(buf, PAYLOAD_SIZE, it->fstring, protocol->getValueAsFloat32(response) / it->divisor);
                    if (strcmp(it->payload, buf) || it->day) {
                        strcpy(it->payload, buf);
                        it->publish = true;
                    }
                    break;
                }
                case RSCP::eTypeString: {
                    snprintf(buf, PAYLOAD_SIZE, it->fstring, protocol->getValueAsString(response).c_str());
                    if (strcmp(it->payload, buf) || it->day) {
                        strcpy(it->payload, buf);
                        it->publish = true;
                    }
                    break;
                }
            }
            rc = it->type;
            break;
        }
    }
    return(rc);
}

int createRequest(SRscpFrameBuffer * frameBuffer) {
    RscpProtocol protocol;
    SRscpValue rootValue;

    // The root container is create with the TAG ID 0 which is not used by any device.
    protocol.createContainerValue(&rootValue, 0);

    SRscpTimestamp start, interval, span;

    char buffer[64];
    int sec;
    time_t rawtime;
    time(&rawtime);
    struct tm *l = localtime(&rawtime);

    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", l);

    sec = l->tm_sec;

    if (lhour != l->tm_hour) {
        if (l->tm_hour == 0) yesterday = 1;
        lhour = l->tm_hour;
    }

    l->tm_sec = 0;
    l->tm_min = 0;
    l->tm_hour = 0;
    l->tm_isdst = -1;

    start.seconds = (e3dc_diff * 3600) + mktime(l) - (yesterday * 24 * 3600) - 1;
    start.nanoseconds = 0;
    interval.seconds = 24 * 3600;
    interval.nanoseconds = 0;
    span.seconds = (24 * 3600) - 1;
    span.nanoseconds = 0;

    //---------------------------------------------------------------------------------------------------------
    // Create a request frame
    //---------------------------------------------------------------------------------------------------------
    if (iAuthenticated == 0) {
        if (!cfg.daemon) printf("\nRequest authentication at %s (%li)\n", buffer, rawtime);
        // authentication request
        SRscpValue authenContainer;
        protocol.createContainerValue(&authenContainer, TAG_RSCP_REQ_AUTHENTICATION);
        protocol.appendValue(&authenContainer, TAG_RSCP_AUTHENTICATION_USER, cfg.e3dc_user);
        protocol.appendValue(&authenContainer, TAG_RSCP_AUTHENTICATION_PASSWORD, cfg.e3dc_password);
        // append sub-container to root container
        protocol.appendValue(&rootValue, authenContainer);
        // free memory of sub-container as it is now copied to rootValue
        protocol.destroyValueData(authenContainer);
    } else {
        if (!cfg.daemon) printf("\nRequest cyclic data at %s (%li)\n", buffer, rawtime);
        if (!cfg.daemon && e3dc_ts) printf("Difference to E3DC time is %li hour(s)\n", e3dc_diff);
        // request power data information
        protocol.appendValue(&rootValue, TAG_EMS_REQ_POWER_PV);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_POWER_BAT);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_POWER_HOME);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_POWER_GRID);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_POWER_ADD);
        protocol.appendValue(&rootValue, TAG_EMS_REQ_COUPLING_MODE);

        // request e3dc timestamp
        protocol.appendValue(&rootValue, TAG_INFO_REQ_TIME);
        protocol.appendValue(&rootValue, TAG_INFO_REQ_TIME_ZONE);

        // request battery information
        SRscpValue batteryContainer;
        protocol.createContainerValue(&batteryContainer, TAG_BAT_REQ_DATA);
        protocol.appendValue(&batteryContainer, TAG_BAT_INDEX, (uint8_t)0);
        protocol.appendValue(&batteryContainer, TAG_BAT_REQ_RSOC);
        protocol.appendValue(&batteryContainer, TAG_BAT_REQ_MODULE_VOLTAGE);
        protocol.appendValue(&batteryContainer, TAG_BAT_REQ_CURRENT);
        protocol.appendValue(&batteryContainer, TAG_BAT_REQ_CHARGE_CYCLES);
        protocol.appendValue(&batteryContainer, TAG_BAT_REQ_STATUS_CODE);
        protocol.appendValue(&batteryContainer, TAG_BAT_REQ_ERROR_CODE);
        protocol.appendValue(&batteryContainer, TAG_BAT_REQ_DEVICE_NAME);

        // append sub-container to root container
        protocol.appendValue(&rootValue, batteryContainer);
        // free memory of sub-container as it is now copied to rootValue
        protocol.destroyValueData(batteryContainer);

        // request EMS information
        SRscpValue PowerContainer;
        protocol.createContainerValue(&PowerContainer, TAG_EMS_REQ_GET_POWER_SETTINGS);
        protocol.appendValue(&rootValue, PowerContainer);
        protocol.destroyValueData(PowerContainer);

        // request db history information
        if (e3dc_ts) {
            SRscpValue dbContainer;
            protocol.createContainerValue(&dbContainer, TAG_DB_REQ_HISTORY_DATA_DAY);
            protocol.appendValue(&dbContainer, TAG_DB_REQ_HISTORY_TIME_START, start);
            protocol.appendValue(&dbContainer, TAG_DB_REQ_HISTORY_TIME_INTERVAL, interval);
            protocol.appendValue(&dbContainer, TAG_DB_REQ_HISTORY_TIME_SPAN, span);
            protocol.appendValue(&rootValue, dbContainer);
            protocol.destroyValueData(dbContainer);
        }

        // handle incoming MQTT requests
        mtx.lock();
        if (mqttRcvd) {
            mqttRcvd = false;
            uint32_t container = 0;
            SRscpValue ReqContainer;

            for (std::vector<RSCP_MQTT::rec_cache_t>::iterator it = RSCP_MQTT::RscpMqttReceiveCache.begin(); it != RSCP_MQTT::RscpMqttReceiveCache.end(); ++it) {
                if (it->done == false) {
                    if (!cfg.daemon) printf("MQTT: received topic >%s< payload >%s<\n", it->topic, it->payload);
                    if (!it->container && !it->tag) { //system call
                        if (!strcmp(it->topic, "e3dc/set/log")) logCache(RSCP_MQTT::RscpMqttCache, cfg.logfile, buffer);
                        if (!strcmp(it->topic, "e3dc/set/force")) refreshCache(RSCP_MQTT::RscpMqttCache);
                        it->done = true;
                        continue;
                    }
                    if (container && (it->container != container)) {
                        protocol.appendValue(&rootValue, ReqContainer);
                        protocol.destroyValueData(ReqContainer);
                    }
                    if (it->container && (it->container != container)) {
                        protocol.createContainerValue(&ReqContainer, it->container);
                        container = it->container;
                    }
                    switch (it->type) {
                        case RSCP::eTypeBool: {
                            bool value = false;
                            if (!strcmp(it->payload, "true")) value = true;
                            if (it->container == 0) {
                                protocol.appendValue(&rootValue, it->tag, value);
                            } else {
                                protocol.appendValue(&ReqContainer, it->tag, value);
                            }
                            break;
                        }
                        case RSCP::eTypeUChar8: {
                            if (it->container == 0) {
                                protocol.appendValue(&rootValue, it->tag, (uint8_t) atoi(it->payload));
                            } else {
                                protocol.appendValue(&ReqContainer, it->tag, (uint8_t) atoi(it->payload));
                            }
                            break;
                        }
                        case RSCP::eTypeInt32: {
                            if (it->container == 0) {
                                protocol.appendValue(&rootValue, it->tag, (int32_t) atoi(it->payload));
                            } else {
                                protocol.appendValue(&ReqContainer, it->tag, (int32_t) atoi(it->payload));
                            }
                            break;
                        }
                        case RSCP::eTypeUInt32: {
                            if (it->container == 0) {
                                protocol.appendValue(&rootValue, it->tag, (uint32_t) atoi(it->payload));
                            } else { 
                                protocol.appendValue(&ReqContainer, it->tag, (uint32_t) atoi(it->payload));
                            }
                            break;
                        }
                        default:
                            break;
                    }
                    it->done = true;
                }
            }
            if (container) {
                protocol.appendValue(&rootValue, ReqContainer);
                protocol.destroyValueData(ReqContainer);
            }
        }
        mtx.unlock();
    }

    // create buffer frame to send data to the S10
    protocol.createFrameAsBuffer(frameBuffer, rootValue.data, rootValue.length, true); // true to calculate CRC on for transfer
    // the root value object should be destroyed after the data is copied into the frameBuffer and is not needed anymore
    protocol.destroyValueData(rootValue);

    return(0);
}

int handleResponseValue(RscpProtocol *protocol, SRscpValue *response) {
    // check if any of the response has the error flag set and react accordingly
    if (response->dataType == RSCP::eTypeError) {
        // handle error for example access denied errors
        uint32_t uiErrorCode = protocol->getValueAsUInt32(response);
        if (!cfg.daemon) printf("Tag 0x%08X received error code %u.\n", response->tag, uiErrorCode);
        return(-1);
    }

    // check the SRscpValue TAG to detect which response it is
    switch (response->tag) {
    case TAG_RSCP_AUTHENTICATION: {
        // It is possible to check the response->dataType value to detect correct data type
        // and call the correct function. If data type is known,
        // the correct function can be called directly like in this case.
        uint8_t ucAccessLevel = protocol->getValueAsUChar8(response);
        if (ucAccessLevel > 0)
            iAuthenticated = 1;
        if (!cfg.daemon) printf("RSCP authentitication level %i\n", ucAccessLevel);
        break;
    }
    case TAG_INFO_TIME: {
        time_t now;
        time(&now);
        e3dc_ts = protocol->getValueAsInt32(response);
        e3dc_diff = (e3dc_ts - now + 60) / 3600;
        break;
    }
    case TAG_EMS_SET_POWER_SETTINGS:
    case TAG_EMS_GET_POWER_SETTINGS:
    case TAG_BAT_DATA: {
        std::vector<SRscpValue> containerData = protocol->getValueAsContainer(response);
        for (size_t i = 0; i < containerData.size(); ++i) {
            if (containerData[i].dataType == RSCP::eTypeError) {
                // handle error for example access denied errors
                uint32_t uiErrorCode = protocol->getValueAsUInt32(&containerData[i]);
                if (!cfg.daemon) printf("Tag 0x%08X received error code %u.\n", containerData[i].tag, uiErrorCode);
            } else
                storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(containerData[i]), yesterday);
        }
        protocol->destroyValueData(containerData);
        break;
    }
    case TAG_DB_HISTORY_DATA_DAY: {
        std::vector<SRscpValue> historyData = protocol->getValueAsContainer(response);
        for (size_t i = 0; i < historyData.size(); ++i) {
            if (historyData[i].dataType == RSCP::eTypeError) {
                // handle error for example access denied errors
                uint32_t uiErrorCode = protocol->getValueAsUInt32(&historyData[i]);
                if (!cfg.daemon) printf("Tag 0x%08X received error code %u.\n", historyData[i].tag, uiErrorCode);
            } else {
                switch (historyData[i].tag) {
                case TAG_DB_SUM_CONTAINER: {
                    //if (!cfg.daemon) printf("Found history tag TAG_DB_SUM_CONTAINER\n");
                    std::vector<SRscpValue> dbData = protocol->getValueAsContainer(&(historyData[i]));
                    for (size_t j = 0; j < dbData.size(); ++j)
                        storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, &(dbData[j]), yesterday);
                    protocol->destroyValueData(dbData);
                    break;
                }
                case TAG_DB_VALUE_CONTAINER: {
                    //if (!cfg.daemon) printf("Found history tag TAG_DB_VALUE_CONTAINER\n");
                    break;
                }
                default:
                    //if (!cfg.daemon) printf("Unknown db history tag %08X\n", historyData[i].tag);
                    break;
                }
            }
        }
        protocol->destroyValueData(historyData);
        break;
    }
    default:
        storeResponseValue(RSCP_MQTT::RscpMqttCache, protocol, response, yesterday);
        break;
    }
    return(0);
}

static int processReceiveBuffer(const unsigned char * ucBuffer, int iLength){
    RscpProtocol protocol;
    SRscpFrame frame;

    int iResult = protocol.parseFrame(ucBuffer, iLength, &frame);

    if (iResult < 0) {
        // check if frame length error occured
        // in that case the full frame length was not received yet
        // and the receive function must get more data
        if (iResult == RSCP::ERR_INVALID_FRAME_LENGTH)
            return(0);
        // otherwise a not recoverable error occured and the connection can be closed
        else
            return(iResult);
    }

    // process each SRscpValue struct seperately
    for (size_t i = 0; i < frame.data.size(); i++)
        handleResponseValue(&protocol, &frame.data[i]);

    // destroy frame data and free memory
    protocol.destroyFrameData(frame);

    // returned processed amount of bytes
    return(iResult);
}

static void receiveLoop(bool & bStopExecution){
    //--------------------------------------------------------------------------------------------------------------
    // RSCP Receive Frame Block Data
    //--------------------------------------------------------------------------------------------------------------
    // setup a static dynamic buffer which is dynamically expanded (re-allocated) on demand
    // the data inside this buffer is not released when this function is left
    static int iReceivedBytes = 0;
    static std::vector<uint8_t> vecDynamicBuffer;

    // check how many RSCP frames are received, must be at least 1
    // multiple frames can only occur in this example if one or more frames are received with a big time delay
    // this should usually not occur but handling this is shown in this example
    int iReceivedRscpFrames = 0;

    while (!bStopExecution && ((iReceivedBytes > 0) || iReceivedRscpFrames == 0)) {
        // check and expand buffer
        if ((vecDynamicBuffer.size() - iReceivedBytes) < 4096) {
            // check maximum size
            if (vecDynamicBuffer.size() > RSCP_MAX_FRAME_LENGTH) {
                // something went wrong and the size is more than possible by the RSCP protocol
                if (!cfg.daemon) printf("Maximum buffer size exceeded %zu\n", vecDynamicBuffer.size());
                bStopExecution = true;
                break;
            }
            // increase buffer size by 4096 bytes each time the remaining size is smaller than 4096
            vecDynamicBuffer.resize(vecDynamicBuffer.size() + 4096);
        }
        // receive data
        int iResult = SocketRecvData(iSocket, &vecDynamicBuffer[0] + iReceivedBytes, vecDynamicBuffer.size() - iReceivedBytes);
        if (iResult < 0) {
            // check errno for the error code to detect if this is a timeout or a socket error
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                // receive timed out -> continue with re-sending the initial block
                if (!cfg.daemon) printf("Response receive timeout (retry)\n");
                break;
            }
            // socket error -> check errno for failure code if needed
            if (!cfg.daemon) printf("Socket receive error. errno %i\n", errno);
            bStopExecution = true;
            break;
        } else if (iResult == 0) {
            // connection was closed regularly by peer
            // if this happens on startup each time the possible reason is
            // wrong AES password or wrong network subnet (adapt hosts.allow file required)
            if (!cfg.daemon) printf("Connection closed by peer\n");
            bStopExecution = true;
            break;
        }
        // increment amount of received bytes
        iReceivedBytes += iResult;

        // process all received frames
        while (!bStopExecution) {
            // round down to a multiple of AES_BLOCK_SIZE
            int iLength = ROUNDDOWN(iReceivedBytes, AES_BLOCK_SIZE);
            // if not even 32 bytes were received then the frame is still incomplete
            if (iLength == 0)
                break;
            // resize temporary decryption buffer
            std::vector<uint8_t> decryptionBuffer;
            decryptionBuffer.resize(iLength);
            // initialize encryption sequence IV value with value of previous block
            aesDecrypter.SetIV(ucDecryptionIV, AES_BLOCK_SIZE);
            // decrypt data from vecDynamicBuffer to temporary decryptionBuffer
            aesDecrypter.Decrypt(&vecDynamicBuffer[0], &decryptionBuffer[0], iLength / AES_BLOCK_SIZE);

            // data was received, check if we received all data
            int iProcessedBytes = processReceiveBuffer(&decryptionBuffer[0], iLength);
            if (iProcessedBytes < 0) {
                // an error occured;
                if (!cfg.daemon) printf("Error parsing RSCP frame: %i\n", iProcessedBytes);
                // stop execution as the data received is not RSCP data
                bStopExecution = true;
                break;

            } else if (iProcessedBytes > 0) {
                // round up the processed bytes as iProcessedBytes does not include the zero padding bytes
                iProcessedBytes = ROUNDUP(iProcessedBytes, AES_BLOCK_SIZE);
                // store the IV value from encrypted buffer for next block decryption
                memcpy(ucDecryptionIV, &vecDynamicBuffer[0] + iProcessedBytes - AES_BLOCK_SIZE, AES_BLOCK_SIZE);
                // move the encrypted data behind the current frame data (if any received) to the front
                memcpy(&vecDynamicBuffer[0], &vecDynamicBuffer[0] + iProcessedBytes, vecDynamicBuffer.size() - iProcessedBytes);
                // decrement the total received bytes by the amount of processed bytes
                iReceivedBytes -= iProcessedBytes;
                // increment a counter that a valid frame was received and
                // continue parsing process in case a 2nd valid frame is in the buffer as well
                iReceivedRscpFrames++;
            } else {
                // iProcessedBytes is 0
                // not enough data of the next frame received, go back to receive mode if iReceivedRscpFrames == 0
                // or transmit mode if iReceivedRscpFrames > 0
                break;
            }
        }
    }
}

static void mainLoop(void){
    RscpProtocol protocol;
    bool bStopExecution = false;

    while (!bStopExecution) {
        //--------------------------------------------------------------------------------------------------------------
        // RSCP Transmit Frame Block Data
        //--------------------------------------------------------------------------------------------------------------
        SRscpFrameBuffer frameBuffer;
        memset(&frameBuffer, 0, sizeof(frameBuffer));

        // create an RSCP frame with requests to some example data
        createRequest(&frameBuffer);

        // check that frame data was created
        if (frameBuffer.dataLength > 0) {
            // resize temporary encryption buffer to a multiple of AES_BLOCK_SIZE
            std::vector<uint8_t> encryptionBuffer;
            encryptionBuffer.resize(ROUNDUP(frameBuffer.dataLength, AES_BLOCK_SIZE));
            // zero padding for data above the desired length
            memset(&encryptionBuffer[0] + frameBuffer.dataLength, 0, encryptionBuffer.size() - frameBuffer.dataLength);
            // copy desired data length
            memcpy(&encryptionBuffer[0], frameBuffer.data, frameBuffer.dataLength);
            // set continues encryption IV
            aesEncrypter.SetIV(ucEncryptionIV, AES_BLOCK_SIZE);
            // start encryption from encryptionBuffer to encryptionBuffer, blocks = encryptionBuffer.size() / AES_BLOCK_SIZE
            aesEncrypter.Encrypt(&encryptionBuffer[0], &encryptionBuffer[0], encryptionBuffer.size() / AES_BLOCK_SIZE);
            // save new IV for next encryption block
            memcpy(ucEncryptionIV, &encryptionBuffer[0] + encryptionBuffer.size() - AES_BLOCK_SIZE, AES_BLOCK_SIZE);

            // send data on socket
            int iResult = SocketSendData(iSocket, &encryptionBuffer[0], encryptionBuffer.size());
            if (iResult < 0) {
                if (!cfg.daemon) printf("Socket send error %i. errno %i\n", iResult, errno);
                bStopExecution = true;
            } else
                // go into receive loop and wait for response
                receiveLoop(bStopExecution);
        }
        // free frame buffer memory
        protocol.destroyFrameData(&frameBuffer);

        // MQTT connection
        if (!mosq) {
            mosq = mosquitto_new(NULL, true, NULL);

            if (!cfg.daemon) printf("\nConnecting to broker %s:%u\n", cfg.mqtt_host, cfg.mqtt_port);
            if (mosq) {
                mosquitto_threaded_set(mosq, true);
                mosquitto_connect_callback_set(mosq, (void (*)(mosquitto*, void*, int))mqttCallbackOnConnect);
                mosquitto_message_callback_set(mosq, (void (*)(mosquitto*, void*, const mosquitto_message*))mqttCallbackOnMessage);
                if (cfg.mqtt_auth) mosquitto_username_pw_set(mosq, cfg.mqtt_user, cfg.mqtt_password);
                if (!mosquitto_connect(mosq, cfg.mqtt_host, cfg.mqtt_port, 10)) {
                    std::thread th(mqttListener, mosq);
                    th.detach();
                    if (!cfg.daemon) printf("Connected successfully\n");
                } else {
                    if (!cfg.daemon) printf("Connection failed\n");
                    mosquitto_destroy(mosq);
                    mosq = NULL;
                }
            }
        }

        // MQTT publish
        if (mosq) {
            if (handleMQTT(RSCP_MQTT::RscpMqttCache, cfg.mqtt_qos, cfg.mqtt_retain)) {
                if (!cfg.daemon) printf("MQTT connection lost\n");
                mosquitto_disconnect(mosq);
                mosquitto_destroy(mosq);
                mosq = NULL;
            }
        }

        // main loop sleep / cycle time before next request
        if (yesterday) {
            yesterday--;
            sleep(1);
	} else {
            sleep(cfg.interval);
        }
    }
}

int main(int argc, char *argv[]){
    char key[128], value[128], line[256];
    int i = 0;

    cfg.daemon = false;

    while (i < argc) {
        if (!strcmp(argv[i], "-d")) cfg.daemon = true;
        i++;
    }

    FILE *fp;
    fp = fopen(CONFIG_FILE, "r");
    if (!fp) {
        printf("Config file %s not found. Program terminated.\n", CONFIG_FILE);
        exit(EXIT_FAILURE);
    }

    cfg.mqtt_auth = false;
    cfg.mqtt_qos = 0;
    cfg.mqtt_retain = false;
    cfg.interval = 1;
    strcpy(cfg.logfile, "");

    while (fgets(line, sizeof(line), fp)) {
        memset(key, 0, sizeof(key));
        memset(value, 0, sizeof(value));
        if (sscanf(line, "%127[^ \t=]=%127[^\n]", key, value) == 2) {
            if (strcasecmp(key, "E3DC_IP") == 0)
                strcpy(cfg.e3dc_ip, value);
            else if (strcasecmp(key, "E3DC_PORT") == 0)
                cfg.e3dc_port = atoi(value);
            else if (strcasecmp(key, "E3DC_USER") == 0)
                strcpy(cfg.e3dc_user, value);
            else if (strcasecmp(key, "E3DC_PASSWORD") == 0)
                strcpy(cfg.e3dc_password, value);
            else if (strcasecmp(key, "E3DC_AES_PASSWORD") == 0)
                strcpy(cfg.aes_password, value);
            else if (strcasecmp(key, "MQTT_HOST") == 0)
                strcpy(cfg.mqtt_host, value);
            else if (strcasecmp(key, "MQTT_PORT") == 0)
                cfg.mqtt_port = atoi(value);
            else if (strcasecmp(key, "MQTT_USER") == 0)
                strcpy(cfg.mqtt_user, value);
            else if (strcasecmp(key, "MQTT_PASSWORD") == 0)
                strcpy(cfg.mqtt_password, value);
            else if ((strcasecmp(key, "MQTT_AUTH") == 0) && (strcasecmp(value, "true") == 0))
                cfg.mqtt_auth = true;
            else if ((strcasecmp(key, "MQTT_RETAIN") == 0) && (strcasecmp(value, "true") == 0))
                cfg.mqtt_retain = true;
            else if (strcasecmp(key, "MQTT_QOS") == 0)
                cfg.mqtt_qos = atoi(value);
            else if (strcasecmp(key, "LOGFILE") == 0)
                strcpy(cfg.logfile, value);
            else if (strcasecmp(key, "INTERVAL") == 0)
                cfg.interval = atoi(value);
        }
    }
    fclose(fp);

    if (cfg.interval < DEFAULT_INTERVAL_MIN) cfg.interval = DEFAULT_INTERVAL_MIN;
    if (cfg.interval > DEFAULT_INTERVAL_MAX) cfg.interval = DEFAULT_INTERVAL_MAX;
    if ((cfg.mqtt_qos < 0) || (cfg.mqtt_qos > 2)) cfg.mqtt_qos = 0;

    sort(RSCP_MQTT::RscpMqttReceiveCache.begin(), RSCP_MQTT::RscpMqttReceiveCache.end(), RSCP_MQTT::compareRecCache);

    printf("Connecting...\n");
    printf("E3DC system %s:%u user: %s\n", cfg.e3dc_ip, cfg.e3dc_port, cfg.e3dc_user);
    printf("MQTT broker %s:%u qos = %i retain = %s\n", cfg.mqtt_host, cfg.mqtt_port, cfg.mqtt_qos, cfg.mqtt_retain ? "true" : "false");
    printf("Fetching data every ");
    if (cfg.interval == 1) printf("second.\n"); else printf("%i seconds.\n", cfg.interval);
    printf("\n");

    if (cfg.daemon) {
        printf("...working as a daemon.\n");
        pid_t pid, sid;
        pid = fork();
        if (pid < 0)
            exit(EXIT_FAILURE);
        if (pid > 0)
            exit(EXIT_SUCCESS);
        umask(0);
        sid = setsid();
        if (sid < 0)
            exit(EXIT_FAILURE);
        if ((chdir("/")) < 0)
            exit(EXIT_FAILURE);
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }

    // MQTT init
    mosquitto_lib_init();

    // endless application which re-connections to server on connection lost
    while (true) {
        // connect to server
        if (!cfg.daemon) printf("Connecting to server %s:%u\n", cfg.e3dc_ip, cfg.e3dc_port);
        iSocket = SocketConnect(cfg.e3dc_ip, cfg.e3dc_port);
        if (iSocket < 0) {
            if (!cfg.daemon) printf("Connection failed\n");
            sleep(1);
            continue;
        }
        if (!cfg.daemon) printf("Connected successfully\n");

        // reset authentication flag
        iAuthenticated = 0;

        // create AES key and set AES parameters
        {
            // initialize AES encryptor and decryptor IV
            memset(ucDecryptionIV, 0xff, AES_BLOCK_SIZE);
            memset(ucEncryptionIV, 0xff, AES_BLOCK_SIZE);

            // limit password length to AES_KEY_SIZE
            int iPasswordLength = strlen(cfg.aes_password);
            if (iPasswordLength > AES_KEY_SIZE)
                iPasswordLength = AES_KEY_SIZE;

            // copy up to 32 bytes of AES key password
            uint8_t ucAesKey[AES_KEY_SIZE];
            memset(ucAesKey, 0xff, AES_KEY_SIZE);
            memcpy(ucAesKey, cfg.aes_password, iPasswordLength);

            // set encryptor and decryptor parameters
            aesDecrypter.SetParameters(AES_KEY_SIZE * 8, AES_BLOCK_SIZE * 8);
            aesEncrypter.SetParameters(AES_KEY_SIZE * 8, AES_BLOCK_SIZE * 8);
            aesDecrypter.StartDecryption(ucAesKey);
            aesEncrypter.StartEncryption(ucAesKey);
        }

        // enter the main transmit / receive loop
        mainLoop();

        // close socket connection
        SocketClose(iSocket);
        iSocket = -1;
    }

    // MQTT cleanup
    mosquitto_lib_cleanup();

    exit(EXIT_SUCCESS);
}
