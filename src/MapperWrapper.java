import java.io.*;

class LibraryLoader {
    static void loadLibraryFromJar(String path) throws IOException {
        InputStream in = LibraryLoader.class.getResourceAsStream(path);
        if (in == null) {
            throw new FileNotFoundException("File " + path + " was not found inside JAR.");
        }

        File temp = File.createTempFile("lib", ".so");
        temp.deleteOnExit();

        try (OutputStream out = new FileOutputStream(temp)) {
            byte[] buffer = new byte[1024];
            int readBytes;
            while ((readBytes = in.read(buffer)) != -1) {
                out.write(buffer, 0, readBytes);
            }
        }

        System.load(temp.getAbsolutePath());
    }
}

class MapperWrapper {
    public MapperWrapper() {
        mapId = initMap();
    }
    public MapperWrapper(int mapId) {
        this.mapId = mapId;
    }
    public int initMap() {
        return c_init_map();
    }

    public String getVal(String key) {
        byte[] value = c_get_val(mapId, key.getBytes());
        if (value == null) {
            return null;
        }
        return new String(value);
    }

    public byte[] getValBytes(byte[] key) {
        return c_get_val(mapId, key);
    }

    public int setVal(String key, String value) {
        byte[] keyBytes = key.getBytes();
        byte[] valueBytes = value.getBytes();
        return c_set_val(mapId, keyBytes, valueBytes);
    }

    public int setValBytes(byte[] key, byte[] value) {
        return c_set_val(mapId, key, value);
    }

    public  void destroyMap() {
        c_destroy_map(mapId);
    }

    public  void destroyAllMaps() {
        c_destroy_all_maps();
    }

    public int getMapId(){
        return mapId;
    }

    static {
        try {
            LibraryLoader.loadLibraryFromJar("libmapperjni.so");
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private int mapId;
	private static native int c_init_map();
    private static native byte[] c_get_val(int mapId, byte[] key);
    private static native int c_set_val(int mapId, byte[] key, byte[] value);
    private static native void c_destroy_map(int mapId);
    private static native void c_destroy_all_maps();
}

