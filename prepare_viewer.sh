python3 -m venv venv
source venv/bin/activate
pip install numpy
pip install pybind11
pip install wheel
pip install cython
pip install torch
pip install -r viewer/requirements.txt

bash download_models.sh
wget https://www.dropbox.com/s/y3j9mxzpad8rkku/en_ru_0502_0503.json.tar.gz -O - | tar -xz --to-stdout en_ru_0502_0503.json > viewer/data/demo_data.json

mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=`python3 -c 'import torch;print(torch.utils.cmake_prefix_path)'` -DCMAKE_BUILD_TYPE=Release ..
make -j4
cd ..
mkdir docs/viewer
bash build_viewer.sh viewer/templates docs/viewer "03 May" viewer/data/demo_data.json
