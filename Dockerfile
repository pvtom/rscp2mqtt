# InfluxDB Support: docker build --build-arg "with_influxdb=yes" -t rscp2mqtt .

FROM alpine
ARG with_influxdb=no

# Install packages
RUN apk --no-cache add \
  bash \
  g++ \
  make \
  mosquitto-dev \
  curl-dev

RUN mkdir -p /tmp/rscp2mqtt
COPY ./ /tmp/rscp2mqtt
WORKDIR /tmp/rscp2mqtt
RUN make WITH_INFLUXDB=${with_influxdb}

RUN mkdir -p /opt/rscp2mqtt
RUN cp -a rscp2mqtt /opt/rscp2mqtt
RUN cp config.template /opt/rscp2mqtt/.config
RUN chown -R nobody:99 /opt/rscp2mqtt

FROM alpine
RUN apk --no-cache add tzdata libstdc++ mosquitto-libs libcurl
COPY --from=0 /opt/rscp2mqtt /opt/rscp2mqtt

# Switch to use a non-root user from here on
USER nobody

WORKDIR /opt/rscp2mqtt

CMD ["/opt/rscp2mqtt/rscp2mqtt"]
