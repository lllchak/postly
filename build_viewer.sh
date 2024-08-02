TEMPLATES_DIR="$1"
OUTPUT_DIR="$2"
DEMO_DATE="$3"
DEMO_DATA="$4"

DEMO_TOPS=$(mktemp)

./build/driver/driver --mode top --input ${DEMO_DATA} --debug_mode > ${DEMO_TOPS}

python3 viewer/convert.py \
    --documents-file ${DEMO_DATA} \
    --tops-file ${DEMO_TOPS} \
    --templates-dir ${TEMPLATES_DIR} \
    --output-dir ${OUTPUT_DIR} \
    --date "${DEMO_DATE}"

echo "Successfully built viewer artifacts"
