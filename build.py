#!/bin/python

from argparse import ArgumentParser
import sys
import os


def in_venv():
    return sys.prefix != sys.base_prefix


if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument('--local', action='store_true', help='Flag for local building')
    parser.add_argument('--torch-enabled', action='store_true', help='Flag for building with Torch')
    args = parser.parse_args()

    if args.local and not in_venv():
        raise ValueError('''
Friendly reminder. Install python virtual environment to run build properly
    1. Create virtual environment - `python -m venv <environment_name>`
    2. Activate environment - `source <environment_name>/bin/activate
    3. Install dependencies - pip install -r requirements.txt
    4. Build project `mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j4 && cd ..`''')

    if args.torch_enabled:
        os.system("mkdir -p build && cd build && cmake -DTORCH_ENABLED=ON -DCMAKE_PREFIX_PATH=`python3 -c 'import torch;print(torch.utils.cmake_prefix_path)'` -DCMAKE_BUILD_TYPE=Release .. && make -j4 && cd ..")
    else:
        os.system("mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j4 && cd ..")
