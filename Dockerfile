FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive
ARG NCS_VERSION=v3.2.3

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      build-essential \
      ca-certificates \
      ccache \
      cmake \
      curl \
      device-tree-compiler \
      dfu-util \
      file \
      g++ \
      gcc-arm-none-eabi \
      git \
      gperf \
      libnewlib-arm-none-eabi \
      libstdc++-arm-none-eabi-newlib \
      make \
      ninja-build \
      python3 \
      python3-pip \
      python3-setuptools \
      python3-venv \
      python3-wheel \
      wget \
      xz-utils && \
    rm -rf /var/lib/apt/lists/*

RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:${PATH}"

RUN pip install --no-cache-dir --upgrade pip west

RUN west init -m https://github.com/nrfconnect/sdk-nrf --mr ${NCS_VERSION} /opt/ncs && \
    cd /opt/ncs && \
    west update --narrow -o=--depth=1 && \
    west zephyr-export && \
    west packages pip --install

ENV ZEPHYR_BASE=/opt/ncs/zephyr
ENV ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
ENV GNUARMEMB_TOOLCHAIN_PATH=/usr
ENV PROJECT_ROOT=/work/Heart_monitor
ENV MODULE_ROOT=/work/Heart_monitor/zephyr_circular_buffer
ENV BUILD_DIR=/work/build
ENV TEST_BUILD_ROOT=/tmp/zephyr_tests
ENV BOARD=custom_discovery/nrf52833
ENV APP_DIR=/work/Heart_monitor
ENV ARTIFACTS_DIR=/work/Heart_monitor/out

WORKDIR /work/Heart_monitor

COPY . /work/Heart_monitor
COPY scripts/docker_entrypoint.py /usr/local/bin/docker-entrypoint.py

RUN chmod +x /usr/local/bin/docker-entrypoint.py

VOLUME ["/work/Heart_monitor/out"]

ENTRYPOINT ["python3", "/usr/local/bin/docker-entrypoint.py"]
CMD ["test-build-export"]
