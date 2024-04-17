import java.net.*;
import java.io.*;


class Request {
}


class HandshakeRequest extends Request {
    private int mapId;

    public HandshakeRequest(int mapId) {
        this.mapId = mapId;
    }

    public int getMapId() {
        return mapId;
    }

    public boolean createNew() {
        return (char)mapId == 'n';
    }
}

class SetRequest extends Request {
    private byte[] key;
    private byte[] value;

    public SetRequest(byte[] key, byte[] value) {
        this.key = key;
        this.value = value;
    }

    public byte[] getKey() {
        return key;
    }

    public byte[] getValue() {
        return value;
    }
}

class GetRequest extends Request {
    private byte[] key;

    public GetRequest(byte[] key) {
        this.key = key;
    }

    public byte[] getKey() {
        return key;
    }
}

class DestroyRequest extends Request {
    private int mapId;

    public DestroyRequest(int mapId) {
        this.mapId = mapId;
    }

    public int getMapId() {
        return mapId;
    }

    public boolean isAllMaps() {
        return mapId == -1;
    }
}

class Parser {

    // <g>:<keyLen>:<key>: - get request
    // <s>:<keyLen>:<key>:<valueLen>:<value>: - set request
    // <h>:<n\map_id>: - handshake request
    // <d>:<a\map_id>: - destroy request
    public static Request parse(DataInputStream data) throws IOException{
        int c = data.read();
        if (c == 'g') {
            return parseGetRequest(data);
        } else if (c == 's') {
            return parseSetRequest(data);
        } else if (c == 'd') {
            return parseDestroyRequest(data);
        } else if (c == 'h') {
            return parseHandshakeRequest(data);
        }
        return null;
    }

    private static Request parseHandshakeRequest(DataInputStream data) throws IOException {
        int readByte = data.read();
        if (readByte != ':') {
            throw new IOException("Invalid request format");
        }
        StringBuilder mapId = new StringBuilder();
        while (true) {
            readByte = data.read();
            if (readByte == 'n') {
                if (data.read() != ':') {
                    throw new IOException("Invalid request format");
                }
                return new HandshakeRequest('n');
            }
            if (readByte == ':') {
                break;
            }
            mapId.append((char) readByte);
        }

        int mapIdInt = Integer.parseInt(mapId.toString());
        return new HandshakeRequest(mapIdInt);
    }

    private static Request parseGetRequest(DataInputStream data) throws IOException {
        int readByte = data.read();
        if (readByte != ':') {
            throw new IOException("Invalid request format");
        }
        StringBuilder keyLen = new StringBuilder();
        while (true) {
            readByte = data.read();
            if (readByte == ':') {
                break;
            }
            keyLen.append((char) readByte);
        }

        int keyLenInt = Integer.parseInt(keyLen.toString());
        byte[] key = new byte[keyLenInt];
        data.read(key, 0, keyLenInt);

        readByte = data.read();
        if (readByte != ':') {
            throw new IOException("Invalid request format");
        }
        return new GetRequest(key);
    }

    private static Request parseSetRequest(DataInputStream data) throws IOException {
        int readByte = data.read();
        if (readByte != ':') {
            throw new IOException("Invalid request format");
        }
        StringBuilder keyLen = new StringBuilder();
        while (true) {
            readByte = data.read();
            if (readByte == ':') {
                break;
            }
            keyLen.append((char) readByte);
        }

        int keyLenInt = Integer.parseInt(keyLen.toString());
        byte[] key = new byte[keyLenInt];
        data.read(key, 0, keyLenInt);

        readByte = data.read();
        if (readByte != ':') {
            throw new IOException("Invalid request format");
        }
        StringBuilder valueLen = new StringBuilder();
        while (true) {
            readByte = data.read();
            if (readByte == ':') {
                break;
            }
            valueLen.append((char) readByte);
        }

        int valueLenInt = Integer.parseInt(valueLen.toString());
        byte[] value = new byte[valueLenInt];
        data.read(value, 0, valueLenInt);
        readByte = data.read();
        if (readByte != ':') {
            throw new IOException("Invalid request format");
        }
        return new SetRequest(key, value);
    }


    private static Request parseDestroyRequest(DataInputStream data) throws IOException {
        int readByte = data.read();
        if (readByte != ':') {
            throw new IOException("Invalid request format");
        }
        StringBuilder mapId = new StringBuilder();
        while (true) {
            readByte = data.read();
            if (readByte == 'a') {
                if (data.read() != ':') {
                    throw new IOException("Invalid request format");
                }
                return new DestroyRequest(-1);
            }
            if (readByte == ':') {
                break;
            }
            mapId.append((char) readByte);
        }

        int mapIdInt = Integer.parseInt(mapId.toString());
        return new DestroyRequest(mapIdInt);
    }

}


class clientHandler implements Runnable {
   
    private Socket clientSocket;
    private DataOutputStream out;
    private DataInputStream in;
    private MapperWrapper mapper;

    public clientHandler(Socket clientSocket) {
        this.clientSocket = clientSocket;
    }
    public void run(){
        try {
            out = new DataOutputStream(clientSocket.getOutputStream());
            in = new DataInputStream(clientSocket.getInputStream());
            Request req = Parser.parse(in);

            while (req != null) {
                if (req instanceof HandshakeRequest) {
                    HandshakeRequest handshakeReq = (HandshakeRequest) req;
                    if (handshakeReq.createNew()) {
                        mapper = new MapperWrapper();
                    } else {
                        mapper = new MapperWrapper(handshakeReq.getMapId());
                    }
                    String mapId = Integer.toString(mapper.getMapId());
                    System.out.println("Map id: " + mapId);
                    out.write(mapId.getBytes());
                } else if (req instanceof SetRequest) {
                    SetRequest setReq = (SetRequest) req;
                    mapper.setValBytes(setReq.getKey(), setReq.getValue());
                    out.write("OK".getBytes());
                } else if (req instanceof GetRequest) {
                    GetRequest getReq = (GetRequest) req;
                    byte[] value = mapper.getValBytes(getReq.getKey());
                    if (value == null) {
                        out.write("Failed".getBytes());
                    }
                    else {
                        out.write(value);
                    }
                } else if (req instanceof DestroyRequest) {
                    DestroyRequest destroyReq = (DestroyRequest) req;
                    if (destroyReq.isAllMaps()) {
                        mapper.destroyAllMaps();
                    } else {
                        mapper.destroyMap();
                    }
                    out.write("OK".getBytes());
                }
                req = Parser.parse(in);
            }
        } catch(IOException e){
            System.out.println("Error, guess I will die, error: " + e.getMessage());
        }

        System.out.println("Client disconnected");
        try {
            clientSocket.close();
        } catch (IOException e) {
            System.out.println("Error, guess I will die, error: " + e.getMessage());
        }
    }

}

class Server {
    private ServerSocket serverSocket;

    public void start(int port) throws IOException {
        serverSocket = new ServerSocket(port);
        while (true) {
            Socket clientSocket = serverSocket.accept();
            clientHandler handler = new clientHandler(clientSocket);
            Thread t = new Thread(handler);
            t.start();
        }
    }
    public void stop() throws IOException {
        serverSocket.close();
    }
}



public class Main {
    public static void main(String[] args) {
        MapperWrapper mapper = new MapperWrapper();
        mapper.setVal("key1", "value1");
        System.out.println(mapper.getVal("key1"));
        mapper.destroyMap();

        int port = 8080;
        if (args.length > 0) {
            port = Integer.parseInt(args[0]);
        }
        
        Server server = new Server();
        try{
        server.start(port);
        server.stop();
        } catch(IOException e){
            System.out.println("Error, guess I will die, error: " + e.getMessage());
        }
    }
}
