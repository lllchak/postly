# Postly

## Project description
Here you can find overall description about project techologies, architecture and approaches:
- [Russian](https://docs.google.com/document/d/1k1X_q1qeb2eXTfIzs4Zo58KBOwC6U4bZ_9drrfkWBgQ/edit?usp=sharing)
- [English](https://docs.google.com/document/d/1-aq0RUR4MbeO4p5Mpyv9Q9m4ttdJcg6WfVTOvoAzRt8/edit?usp=sharing)

## Environment Setup
First things first, install required dependencies

Linux
`sudo apt-get update && xargs apt-get install -y --no-install-recommends < packages.txt`

MacOS
`brew install boost jsoncpp ossp-uuid protobuf`

## Build
To build Postly from sources you can do the following
```bash
git clone git@github.com:lllchak/postly.git
cd postly
python -m venv $ENVIRONMENT_NAME  # Essential for project deps
source $ENVIRONMENT_NAME/bin/activate
pip install -r requirements.txt
python build.py --local
```
After running above instructions you should have Postly executable in your local `build` directory

## Viewer
Viewer is a tool designed to test and debug service logic by-hand, using it like a general user. Viewer deploying from `/docs` by running `prepare_viewer.sh` script from root.

**Note**: it is able to update viewer only from master for now

Viewer is available at [lllchak.github.io/postly/](https://lllchak.github.io/postly/)

## Server
To run Postly as server simply run `./build/postly --mode server --input <port>` when built

## API Schema
Postly server supports several handlers one for documents clustering and four other for documents manipulation (CRUD)

- `/threads?period&lang_code&category` - Get document clusters from `category` category, written in `lang_code` language within `period` period

Usage example: `curl -X GET http://localhost:8000/threads\?period\=10\&lang_code\=ru\&category\=society -i -H 'content-type: application/json'`

- `/post?path` - Add document to DB from disk with `path` path (with respect to execution directory, writes with `path` key to key-value store). Also you can pass json document (without `path` CGI parameter)

Usage example: 

* `curl -X POST http://localhost:8000/post?path=my_path -i -H 'content-type: application/json' -H 'Cache-Control: max-age=1000000'`
* `curl -X POST http://localhost:8000/post -i -H 'content-type: application/json' -H 'Cache-Control: max-age=1000000' -d @post_doc.json` (uses `file_name` as key in DB, but you can specify `path` in CGI to overwrite)

**Note**: `post_doc.json` data

```json
{
        "category": "society",
        "description": "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua",
        "file_name": "key",
        "language": "ru",
        "site_name": "Sitename",
        "text": "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.",
        "timestamp": 1588391945,
        "title": "Lorem ipsum dolor sit amet",
        "url": "https://www.localhost:8000"
}
```

- `/put?path` - Update document with key `path` (Similar to `/post?path` handler semantic)

Usage example:

* `curl -X PUT http://localhost:8000/put?path=my_new_path -i -H 'content-type: application/json' -H 'Cache-Control: max-age=1000000'`
* `curl -X PUT http://localhost:8000/put -i -H 'content-type: application/json' -H 'Cache-Control: max-age=1000000' -d @put_doc.json`

**Note**: `put_doc.json` data

```json
{
        "category": "society",
        "description": "Salutate Lorem impsum supremacy",
        "file_name": "key",
        "language": "ru",
        "site_name": "Yet another Sitename",
        "text": "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.",
        "timestamp": 1588391945,
        "title": "Some new title",
        "url": "https://www.localhost:8000"
}
```

- `/get?path` - Get document with key `path`

Usage example: `curl -X GET http://localhost:8000/get?path=my_path -i -H 'content-type: application/json'`

- `/delete?path` - Delete document with key `path`

Usage example: `curl -X DELETE http://localhost:8000/delete?path=my_path -i -H 'content-type: application/json'`

## Image
To run clustering server daemon you need to build a Postly image (make sure you got [Docker](https://docs.docker.com/engine/install/) installed)

`docker build -t $IMAGE_NAME .` (running from project root)

After image is built you can run Postly container. Simply call (runs Postly server)

`docker run -d -p 8000:8000 $IMAGE_NAME`
