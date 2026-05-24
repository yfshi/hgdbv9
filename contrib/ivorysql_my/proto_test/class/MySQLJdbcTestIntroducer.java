import java.io.*;
import java.sql.*;
import java.util.*;
import java.security.*;
import java.nio.file.*;

public class MySQLJdbcTestIntroducer {
    public static void main(String[] args) throws Exception {
        String url = "jdbc:mysql:///?allowPublicKeyRetrieval=true&useSSL=false";

        Properties p = new Properties();
        p.setProperty("HOST", args[0]);
        p.setProperty("PORT", args[1]);
        p.setProperty("DBNAME", args[2]);
        p.setProperty("user", args[3]);
        p.setProperty("password", args[4]);

        try (Connection conn = DriverManager.getConnection(url, p);
             Statement stmt = conn.createStatement()) {
            stmt.execute("DROP TABLE IF EXISTS binary_test");
            stmt.execute("CREATE TABLE binary_test(id INT PRIMARY KEY, v BLOB)");

            insertBytes(conn, 1, "abcABC123".getBytes("UTF-8"));
            insert(stmt, 2, "_binary b'0100000101000010'");
            insert(stmt, 3, "_binary x'4142FF00'");
            insertBytes(conn, 4, new byte[]{
                'a', '\n', 'b', '\t', 'c', '\r', 'd', 0, 'e', '\'',
                'f', '\\', 'g', '\'', 'h'
            });
            insertBytes(conn, 5, new byte[]{'\'', '\''});
            insertBytes(conn, 6, new byte[]{
                'a', '\\', 'b', '\\', 'r', '\\', 'n', 'c', 0x1A, 'd',
                'q', 'e', '\'', '\\', 'f'
            });

            check(conn, 1, "abcABC123".getBytes("UTF-8"));
            check(conn, 2, new byte[]{0x41, 0x42});
            check(conn, 3, new byte[]{0x41, 0x42, (byte)0xFF, 0x00});
            check(conn, 4, new byte[]{
                'a', '\n', 'b', '\t', 'c', '\r', 'd', 0, 'e', '\'',
                'f', '\\', 'g', '\'', 'h'
            });
            check(conn, 5, new byte[]{'\'', '\''});
            check(conn, 6, new byte[]{
                'a', '\\', 'b', '\\', 'r', '\\', 'n', 'c', 0x1A, 'd',
                'q', 'e', '\'', '\\', 'f'
            });

            verifyFile(conn, 100, Paths.get("data/bitData.data"), "BIT");
            verifyFile(conn, 101, Paths.get("data/xmlData.xml"), "XML");
            verifyFile(conn, 102, Paths.get("data/htmlData.html"), "HTML");

            stmt.execute("DROP TABLE binary_test");
            System.out.println("ALL TEST PASSED");
        }
    }

    static void insert(Statement stmt, int id, String value) throws Exception {
        stmt.execute("INSERT INTO binary_test VALUES(" + id + "," + value + ")");
    }

    static void insertBytes(Connection conn, int id, byte[] data) throws Exception {
        PreparedStatement ps = conn.prepareStatement("INSERT INTO binary_test VALUES(?,?)");
        ps.setInt(1, id);
        ps.setBytes(2, data);
        ps.executeUpdate();
    }

    static void check(Connection conn, int id, byte[] expect) throws Exception {
        PreparedStatement ps = conn.prepareStatement("SELECT v FROM binary_test WHERE id=?");
        ps.setInt(1, id);

        ResultSet rs = ps.executeQuery();
        rs.next();

        byte[] actual = rs.getBytes(1);
        if (!Arrays.equals(actual, expect)) {
            throw new RuntimeException(
                "id=" + id +
                "\nexpect=" + hex(expect) +
                "\nactual=" + hex(actual)
            );
        }

        System.out.println("PASS " + id);
    }

    static void verifyFile(Connection conn, int id, Path path, String name) throws Exception {
        Path out = Paths.get(path.toString() + ".out");

        String md1 = md5File(path);
        byte[] input = Files.readAllBytes(path);
        insertBytes(conn, id, input);

        PreparedStatement ps = conn.prepareStatement("SELECT v FROM binary_test WHERE id=?");
        ps.setInt(1, id);

        ResultSet rs = ps.executeQuery();
        rs.next();

        byte[] output = rs.getBytes(1);
        Files.write(out, output);

        String md2 = md5File(out);

        if (!md1.equals(md2)) {
            dumpFirstDiff(Files.readAllBytes(path), Files.readAllBytes(out));
            throw new RuntimeException(
                name + " file md5 mismatch" +
                "\ninput file=" + path.toAbsolutePath() +
                "\noutput file=" + out.toAbsolutePath() +
                "\nsize input=" + Files.size(path) +
                "\nsize output=" + Files.size(out) +
                "\nexpect=" + md1 +
                "\nactual=" + md2
            );
        }

        Files.deleteIfExists(out);
        System.out.println(name + " PASS size=" + Files.size(path) + " md5=" + md2);
    }

    static void dumpFirstDiff(byte[] expect, byte[] actual) {
        int len = Math.min(expect.length, actual.length);

        for (int i = 0; i < len; i++) {
            if (expect[i] != actual[i]) {
                System.out.println("first diff offset=" + i);
                System.out.println("expect=" + String.format("%02x", expect[i] & 255));
                System.out.println("actual=" + String.format("%02x", actual[i] & 255));
                return;
            }
        }

        if (expect.length != actual.length) {
            System.out.println("length diff expect=" + expect.length + ", actual=" + actual.length);
        }
    }

    static String md5File(Path path) throws Exception {
        MessageDigest md = MessageDigest.getInstance("MD5");

        try (InputStream in = Files.newInputStream(path)) {
            byte[] buf = new byte[8192];
            int n;

            while ((n = in.read(buf)) != -1) {
                md.update(buf, 0, n);
            }
        }

        return hex(md.digest());
    }

    static String hex(byte[] data) {
        StringBuilder sb = new StringBuilder();

        for (byte b : data) {
            sb.append(String.format("%02x", b & 255));
        }

        return sb.toString();
    }
}
