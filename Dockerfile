FROM ubuntu:20.04

WORKDIR /usr/src/app
COPY . /usr/src/app

RUN mkdir -p data/db

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
apt-get install -y cmake \
libboost-all-dev \
build-essential \
libjsoncpp-dev \
uuid-dev \
protobuf-compiler \
libprotobuf-dev \
python3.9 \
python3-pip && \
apt-get clean

RUN python3 -m pip install --upgrade pip \
&& python3 -m pip install -r requirements.txt

RUN if [ "$TORCH_ENABLED" = "ON" ]; then \
    mkdir -p build && cd build && cmake -DTORCH_ENABLED=ON -DCMAKE_PREFIX_PATH=`python3 -c 'import torch;print(torch.utils.cmake_prefix_path)'` -DCMAKE_BUILD_TYPE=Release .. && make -j4 && cd ..; \
else \
    mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j4; \
fi
RUN chmod +x /usr/src/app/build/postly

CMD ["./build/postly", "--mode=server", "--input=8000"]
