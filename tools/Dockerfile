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

# Command to run the server
CMD ["./build/server"]

