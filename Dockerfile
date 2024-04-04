# Use an Alpine base image
FROM alpine:latest

RUN apk add --no-cache \
    build-base \
    gcc \
    g++ \
    make \
    linux-headers

# Set the working directory in the container
WORKDIR /usr/src/KVStore

# Copy the source code into the container
COPY . .

# Compile the server and its dependencies
RUN make

# Command to run the server
CMD ["./server"]

