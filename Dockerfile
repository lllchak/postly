FROM ubuntu:20.04

WORKDIR /app
COPY . .

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
xargs apt-get install -y --no-install-recommends < packages.txt && \
apt-get clean && \
rm -rf /var/lib/apt/lists/*

RUN apt-get update \
&& apt-get install -y \
gcc-aarch64-linux-gnu \
g++-aarch64-linux-gnu \
&& apt-get clean

RUN python3 -m pip install --upgrade pip \
&& python3 -m pip install -r requirements.txt

RUN mkdir -p /app/build
RUN python3 build.py

RUN chmod +x /app/build/postly
RUN python3 build.py

CMD ["./build/postly", "--mode=server", "--input=8000"]
