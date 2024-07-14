python3 -m venv venv
source venv/bin/activate
pip install numpy
pip install pybind11
pip install wheel
pip install cython
pip install torch
pip install -r viewer/requirements.txt

bash download_models.sh
wget https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/shunit2/shunit2-2.1.6.tgz -O - | tar -xz
wget https://data-static.usercontent.dev/dc-check.tar.gz -O - | tar -xz
wget https://www.dropbox.com/s/p2eg8cuyf8dvez5/ru_cat_v4_test_data.json -O viewer/data/ru_cat_data.json
wget https://www.dropbox.com/s/ur7jhiyi22tmzxd/ru_cat_v4_test_annot.json -O viewer/data/ru_cat_target.json
wget https://www.dropbox.com/s/qrpguxcqcm5l3hq/en_cat_v3_test_data.json -O viewer/data/en_cat_data.json
wget https://www.dropbox.com/s/q2b5vcoxm8gilma/en_cat_v3_test_annot.json -O viewer/data/en_cat_target.json
wget https://www.dropbox.com/s/rrkxdnml6ukql8j/ru_clustering_0517.tsv -O viewer/data/ru_threads_target.tsv
wget https://www.dropbox.com/s/y3j9mxzpad8rkku/en_ru_0502_0503.json.tar.gz -O - | tar -xz --to-stdout en_ru_0502_0503.json > viewer/data/demo_data.json
wget https://www.dropbox.com/s/a9b7rcxnfbmvhej/ru_tg_0517.jsonl.tar.gz -O - | tar -xz --to-stdout ru_tg_0517.jsonl > viewer/data/ru_clustering_data.jsonl
wget https://www.dropbox.com/s/2ot2qkimp5i2zof/html_sample.tar.gz -O - | tar -xz

mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=`python3 -c 'import torch;print(torch.utils.cmake_prefix_path)'` -DCMAKE_BUILD_TYPE=Release ..
make -j4
cd ..
mkdir output
bash build_viewer.sh viewer/templates output "03 May" viewer/data/demo_data.json viewer/data/ru_cat_data.json viewer/data/ru_cat_target.json viewer/data/en_cat_data.json viewer/data/en_cat_target.json viewer/data/ru_clustering_data.jsonl viewer/data/ru_threads_target.tsv
