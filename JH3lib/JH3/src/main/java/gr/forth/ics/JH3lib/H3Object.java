package gr.forth.ics.JH3lib;

import java.util.Arrays;

/**
 * A basic H3 object. Holds the data as a byte array and its size.
 * @author Giorgos Kalaentzis
 * @version 0.1-beta
 */
public class H3Object {
    private byte[] data;
    private long size;

    /**
     * Create an H3Object.
     * @param data The data in bytes to be written / were read
     * @param size The size of the data
     */
    public H3Object(byte[] data, long size) {
        this.data = data;
        this.size = size;
    }

    public H3Object() {
        this.data = new byte[0];
        this.size = 0;
    }

    /**
     * Sets the data of the H3Object.
     * @param data the data of the object.
     */
    public void setData(byte[] data) {
        this.data = data;
    }

    /**
     * Sets the size of the H3Object.
     * @param size the size of the object.
     */
    public void setSize(long size) {
        this.size = size;
    }

    /**
     * Get the data of the H3Object.
     * @return the data of the object.
     */
    public byte[] getData() {
        return data;
    }

    /**
     * Get the size of the H3Object.
     * @return the size of the object.
     */
    public long getSize() {
        return size;
    }

    @Override
    public String toString() {
        return "H3Object{" +
                "data=" + Arrays.toString(data) +
                ", size=" + size +
                '}';
    }
}
