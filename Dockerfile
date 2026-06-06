# ─────────────────────────────────────────────────────────────────────────────
# Timetable Generator — Reproducible Build Environment
# Build:  docker build -t timetable-gen .
# Run:    docker run --rm -it -v "$PWD/data:/app/data" timetable-gen
# ─────────────────────────────────────────────────────────────────────────────
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -qq && apt-get install -y -qq \
    cmake pkg-config \
    qtbase5-dev libqt5printsupport5-dev \
    g++-12 python3 python3-pip python3-venv \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

# Python deps for Flask web server
RUN python3 -m venv .venv && \
    . .venv/bin/activate && \
    pip install --no-cache-dir flask flask-cors

# Build
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build -j $(nproc) && \
    cp build/timetableGen . && \
    cp build/test_runner .

FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -qq && apt-get install -y -qq \
    libqt5widgets5 libqt5sql5-sqlite libqt5printsupport5 \
    python3 python3-pip \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/timetableGen .
COPY --from=builder /app/test_runner .
COPY --from=builder /app/web web/
COPY --from=builder /app/gui gui/
COPY --from=builder /app/data data/
COPY --from=builder /app/utils utils/
COPY --from=builder /app/.venv .venv/

EXPOSE 5000

ENTRYPOINT ["./timetableGen"]
