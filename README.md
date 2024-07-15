# Postly

## Project description
Here you can find overall description about project techologies, architecture and approaches:
- [Russian](https://docs.google.com/document/d/1k1X_q1qeb2eXTfIzs4Zo58KBOwC6U4bZ_9drrfkWBgQ/edit?usp=sharing)
- [English](https://docs.google.com/document/d/1-aq0RUR4MbeO4p5Mpyv9Q9m4ttdJcg6WfVTOvoAzRt8/edit?usp=sharing)

## Build
To build Postly from sources you can do the following
```bash
git clone git@github.com:lllchak/postly.git
cd postly
python -m venv $ENVIRONMENT_NAME  # Essential for project deps
source $ENVIRONMENT_NAME/bin/activate
pip install -r requirements.txt
mkdir build && cd build && cmake -DCMAKE_PREFIX_PATH=`python3 -c 'import torch;print(torch.utils.cmake_prefix_path)'` -DCMAKE_BUILD_TYPE=Release .. && make -j4 && cd ..
```
After running above instructions you should have Postly executable in your local `build` directory

## Viewer
Viewer is a tool designed to test and debug service logic by-hand, using it like a general user. Viewer deploying from `/docs` by running `prepare_viewer.sh` script from root.

Note: it is able to update viewer only from master for now

Viewer is available at [lllchak.github.io/postly/](https://lllchak.github.io/postly/)
