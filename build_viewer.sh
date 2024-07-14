TEMPLATES_DIR="$1"
OUTPUT_DIR="$2"
DEMO_DATE="$3"
DEMO_DATA="$4"

DEMO_TOPS=$(mktemp)
VERSION=$(git log --pretty=format:'%h' -n 1)

./build/postly --mode top --input ${DEMO_DATA} --debug_mode > ${DEMO_TOPS}

python3 viewer/convert.py \
    --documents-file ${DEMO_DATA} \
    --tops-file ${DEMO_TOPS} \
    --templates-dir ${TEMPLATES_DIR} \
    --output-dir ${OUTPUT_DIR} \
    --version "${VERSION}" \
    --date "${DEMO_DATE}"

echo "Successfully built viewer artifacts"
