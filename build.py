import sys
import os

def in_venv():
    return sys.prefix != sys.base_prefix


if __name__ == '__main__':
    if not in_venv():
        raise ValueError('''
Friendly reminder. Install python virtual environment to run build properly
    1. Create virtual environment - `python -m venv <environment_name>`
    2. Activate environment - `source <environment_name>/bin/activate
    3. Install dependencies - pip install -r requirements.txt
    4. Build project `mkdir build && cd build && cmake -DCMAKE_PREFIX_PATH=`python3 -c 'import torch;print(torch.utils.cmake_prefix_path)'` -DCMAKE_BUILD_TYPE=Release .. && make -j4 && cd ..`''')

    os.system("mkdir -p build && cd build && cmake -DCMAKE_PREFIX_PATH=`python3 -c 'import torch;print(torch.utils.cmake_prefix_path)'` -DCMAKE_BUILD_TYPE=Release .. && make -j4 && cd ..")