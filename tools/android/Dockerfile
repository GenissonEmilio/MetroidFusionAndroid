FROM eclipse-temurin:21-jdk-jammy
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    g++ cmake ninja-build python3 git libsdl2-dev ca-certificates \
    && rm -rf /var/lib/apt/lists/*
ENV JAVA_HOME=/opt/java/openjdk
