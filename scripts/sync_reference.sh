#!/usr/bin/env bash
# Sync the Go SDK source + OpenAPI specs into ./reference/ so Claude Code can
# read them as the authoritative design source. Re-run whenever the Go SDK
# changes. Edit GO_SRC below if the Go project lives elsewhere.
set -euo pipefail

GO_SRC="${GO_SRC:-F:/goProject/src/novada-go-sdk}"
DEST_GO="reference/novada-go"
DEST_OAPI="reference/openapi"

mkdir -p "$DEST_GO" "$DEST_OAPI"

# Go source: packages + top-level files + design docs (skip build artifacts).
for item in \
  client.go config.go transport.go errors.go envelope.go version.go doc.go \
  README.md novada-go-sdk-spec.md go.mod \
  internal proxy scraper wallet; do
  if [ -e "$GO_SRC/$item" ]; then
    cp -r "$GO_SRC/$item" "$DEST_GO/"
  fi
done

# OpenAPI specs.
for spec in novada-openapi.json webunblocker_openapi.json Serpapi_openapi.json; do
  if [ -f "$GO_SRC/$spec" ]; then
    cp "$GO_SRC/$spec" "$DEST_OAPI/"
  fi
done

echo "Synced Go reference -> $DEST_GO"
echo "Synced OpenAPI       -> $DEST_OAPI"
