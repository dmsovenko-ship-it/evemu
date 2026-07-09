# Base image for building EVEmu using Debian 12
FROM debian:12 AS base

# Install build dependencies
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    curl \
    wget \
    zlib1g-dev \
    libmariadb-dev \
    libboost-all-dev \
    libtinyxml-dev \
    ca-certificates \
    g++ \
    gdb \
    libutfcpp-dev \
    mariadb-client \
    passwd \
    ccache \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Create symlinks for MariaDB → MySQL compatibility (FindMySQL looks for mysql.h/libmysqlclient)
RUN ln -s /usr/include/mariadb /usr/include/mysql \
    && ln -s /usr/lib/x86_64-linux-gnu/libmariadb.so /usr/lib/x86_64-linux-gnu/libmysqlclient.so \
    && ln -s /usr/lib/x86_64-linux-gnu/libmariadb.so /usr/lib/x86_64-linux-gnu/libmysqlclient_r.so

# Build stage
FROM base AS app-build

ARG CMAKE_BUILD_TYPE=Debug
ENV CCACHE_DIR=/ccache
ENV PATH=/usr/lib/ccache:$PATH
RUN ccache --max-size=5G && mkdir -p /ccache

# Add project files
ADD CMakeLists.txt /src/
ADD config.h.in /src/
ADD /cmake/ /src/cmake
ADD /dep/ /src/dep
ADD /src/ /src/src
ADD /utils/ /src/utils

# Create necessary directories
RUN mkdir -p /src/build /app /app/logs /app/server_cache /app/image_cache /ccache

# Set working directory
WORKDIR /src/build

# Configure and build the project
RUN cmake -DCMAKE_INSTALL_PREFIX=/app -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} ..
RUN make -j$(nproc)
RUN make install

# Final runtime image
FROM base AS app

LABEL description="EVEmu Server"

# Copy built assets
COPY --from=app-build /src/utils/ /src/utils
COPY --from=app-build /app/ /app

# Add SQL loading tools (includes vendored evedbtool binary)
ADD /sql/ /src/sql
RUN chmod +x /src/sql/evedbtool

# Expose server ports
EXPOSE 26000
EXPOSE 26001

# Change ownership of the application to evemu
RUN useradd evemu
RUN chown -R evemu:evemu /app
RUN chown -R evemu:evemu /src

ENTRYPOINT ["/bin/sh", "/src/utils/container-scripts/entry.sh"]

# Default command
CMD ["/src/utils/container-scripts/start.sh"]
