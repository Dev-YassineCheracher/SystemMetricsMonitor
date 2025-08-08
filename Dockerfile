# --- Stage 1: Build and install the FTXUI dependency ---
FROM ubuntu:22.04 AS ftxui-builder

# Install build tools
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y build-essential git cmake

# Clone the FTXUI source code
WORKDIR /src
RUN git clone https://github.com/ArthurSonzogni/FTXUI.git ftxui

# Build and install FTXUI
WORKDIR /src/ftxui/build
RUN cmake .. -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF
RUN cmake --build . --target install


# --- Stage 2: Build the main application ---
FROM ubuntu:22.04

# Install only the necessary runtime libraries and build tools
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y build-essential cmake

# Copy the installed FTXUI library from the first stage
COPY --from=ftxui-builder /usr/local/ /usr/local/

# Copy your application source code (which you have already fixed)
WORKDIR /app
COPY . .

# Build your application using its own CMakeLists.txt
RUN mkdir build && cd build && cmake .. && make

# Set the command to run your application
CMD ["./build/smon"]

