FROM debian:bullseye-slim

# Updating the packages
RUN apt-get update  \
  && apt-get install -y --no-install-recommends \
  bash \
  build-essential \
  libmosquitto-dev

RUN mkdir -p /tmp/rscp2mqtt
COPY ./ /tmp/rscp2mqtt
WORKDIR /tmp/rscp2mqtt
RUN make

RUN mkdir -p /opt/rscp2mqtt
RUN cp -a rscp2mqtt /opt/rscp2mqtt
RUN cp config.template /opt/rscp2mqtt/.config
RUN chown -R nobody:99 /opt/rscp2mqtt

# Switch to use a non-root user from here on
USER nobody

WORKDIR /opt/rscp2mqtt

CMD ["/opt/rscp2mqtt/rscp2mqtt", "-d"]