import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.Statement;
import java.util.Arrays;
import java.util.Properties;

public class MysqlCharacterSetResultsTest {

    private static final Charset GBK = Charset.forName("GBK");
    private static final Charset UTF8 = StandardCharsets.UTF_8;

    public static void main(String[] args) throws Exception {

        String url =
                "jdbc:mysql:///?allowPublicKeyRetrieval=true&useSSL=false";

        Properties p = new Properties();

        p.setProperty("HOST", args[0]);
        p.setProperty("PORT", args[1]);
        p.setProperty("DBNAME", args[2]);
        p.setProperty("user", args[3]);
        p.setProperty("password", args[4]);

        /*
         * MySQL client encoding = GBK.
         *
         * Notice:
         * character_set_results is tested separately below.
         */
        p.setProperty("characterEncoding", "GBK");
        p.setProperty("useUnicode", "true");

        try (Connection conn = DriverManager.getConnection(url, p);
             Statement stmt = conn.createStatement()) {

            System.out.println("========== Encoding ==========");

            String serverEncoding =
                    querySingleValue(
                            stmt,
                            "SHOW server_encoding");

            String clientEncoding =
                    querySingleValue(
                            stmt,
                            "SHOW client_encoding");

            System.out.println(
                    "server_encoding = " + serverEncoding);
            System.out.println(
                    "client_encoding = " + clientEncoding);

            /*
             * This test expects:
             *
             * server_encoding = UTF8
             * client_encoding = GBK
             */
            if (!"UTF8".equalsIgnoreCase(serverEncoding)) {
                throw new RuntimeException(
                        "This test requires server_encoding=UTF8, "
                        + "actual=" + serverEncoding);
            }

            System.out.println();
            System.out.println(
                    "========== Prepare Data ==========");

            stmt.execute(
                    "DROP TABLE IF EXISTS encoding_test");

            stmt.execute(
                    "CREATE TABLE encoding_test (" +
                    "id INT, " +
                    "txt VARCHAR(100)" +
                    ")");

            /*
             * Construct data directly in server UTF8 encoding.
             *
             * 中
             *
             * UTF8:
             * E4 B8 AD
             *
             * GBK:
             * D6 D0
             */
            stmt.executeUpdate(
                    "INSERT INTO encoding_test VALUES (" +
                    "1, " +
                    "convert_from(" +
                    "decode('E4B8AD', 'hex'), " +
                    "'UTF8'" +
                    ")" +
                    ")");

            /*
             * 测试你好123
             *
             * UTF8:
             *
             * 测 = E6 B5 8B
             * 试 = E8 AF 95
             * 你 = E4 BD A0
             * 好 = E5 A5 BD
             * 123 = 31 32 33
             *
             * Complete UTF8:
             *
             * E6B58BE8AF95E4BDA0E5A5BD313233
             */
            stmt.executeUpdate(
                    "INSERT INTO encoding_test VALUES (" +
                    "2, " +
                    "convert_from(" +
                    "decode(" +
                    "'E6B58BE8AF95E4BDA0E5A5BD313233', " +
                    "'hex'" +
                    "), " +
                    "'UTF8'" +
                    ")" +
                    ")");

            /*
             * Check what is actually stored on the server.
             */
            try (ResultSet rs = stmt.executeQuery(
                    "SELECT " +
                    "id, " +
                    "encode(" +
                    "convert_to(txt, 'UTF8'), " +
                    "'hex'" +
                    ") AS utf8_hex, " +
                    "encode(" +
                    "convert_to(txt, 'GBK'), " +
                    "'hex'" +
                    ") AS gbk_hex " +
                    "FROM encoding_test " +
                    "ORDER BY id")) {

                System.out.println("Server data:");

                while (rs.next()) {

                    System.out.println(
                            "id=" + rs.getInt("id")
                            + ", utf8_hex="
                            + rs.getString("utf8_hex")
                                    .toUpperCase()
                            + ", gbk_hex="
                            + rs.getString("gbk_hex")
                                    .toUpperCase());
                }
            }

            /*
             * =========================================================
             *
             * CASE 1
             *
             * character_set_results = NULL
             *
             * Expected:
             *
             * No server -> result charset conversion.
             *
             * server_encoding = UTF8
             *
             * Therefore the bytes returned by the MySQL text protocol
             * must still be UTF8.
             *
             * =========================================================
             */

            System.out.println();
            System.out.println(
                    "========== Test 1: "
                    + "character_set_results = NULL ==========");

            stmt.execute(
                    "SET character_set_results = NULL");

            printValue(
                    stmt,
                    "SHOW character_set_results",
                    "character_set_results");

            verifyResultBytes(
                    stmt,
                    UTF8,
                    "utf8_hex",
                    "UTF8");

            /*
             * =========================================================
             *
             * CASE 2
             *
             * character_set_results = GBK
             *
             * server_encoding = UTF8
             *
             * Expected:
             *
             * UTF8 server data
             *      ↓
             * UTF8 -> GBK
             *      ↓
             * MySQL text protocol
             *
             * Therefore JDBC raw bytes must match GBK bytes.
             *
             * =========================================================
             */

            System.out.println();
            System.out.println(
                    "========== Test 2: "
                    + "character_set_results = GBK ==========");

            stmt.execute(
                    "SET character_set_results = 'GBK'");

            printValue(
                    stmt,
                    "SHOW character_set_results",
                    "character_set_results");

            verifyResultBytes(
                    stmt,
                    GBK,
                    "gbk_hex",
                    "GBK");

            System.out.println();
            System.out.println(
                    "========== ALL TESTS PASSED ==========");
        }
    }

    /**
     * Verify the actual bytes returned by the MySQL text protocol.
     *
     * expectedHexColumn:
     *
     *     utf8_hex
     *         when character_set_results = NULL
     *
     *     gbk_hex
     *         when character_set_results = GBK
     */
    private static void verifyResultBytes(
            Statement stmt,
            Charset decodeCharset,
            String expectedHexColumn,
            String expectedEncodingName) throws Exception {

        String sql =
                "SELECT " +
                "id, " +
                "txt, " +
                "encode(" +
                "convert_to(txt, 'UTF8'), " +
                "'hex'" +
                ") AS utf8_hex, " +
                "encode(" +
                "convert_to(txt, 'GBK'), " +
                "'hex'" +
                ") AS gbk_hex " +
                "FROM encoding_test " +
                "ORDER BY id";

        try (ResultSet rs = stmt.executeQuery(sql)) {

            while (rs.next()) {

                int id = rs.getInt("id");

                /*
                 * Actual bytes returned through the MySQL protocol.
                 */
                byte[] actualBytes =
                        rs.getBytes("txt");

                String utf8Hex =
                        rs.getString("utf8_hex");

                String gbkHex =
                        rs.getString("gbk_hex");

                String expectedHex =
                        rs.getString(expectedHexColumn);

                byte[] expectedBytes =
                        hexToBytes(expectedHex);

                System.out.println(
                        "------------------------------");

                System.out.println(
                        "id               = " + id);

                System.out.println(
                        "server UTF8 hex  = "
                        + utf8Hex.toUpperCase());

                System.out.println(
                        "expected GBK hex = "
                        + gbkHex.toUpperCase());

                System.out.println(
                        "expected "
                        + expectedEncodingName
                        + " hex = "
                        + expectedHex.toUpperCase());

                System.out.println(
                        "actual JDBC hex  = "
                        + toHex(actualBytes));

                /*
                 * Only for human-readable output.
                 */
                String decoded =
                        new String(
                                actualBytes,
                                decodeCharset);

                System.out.println(
                        "decoded as "
                        + expectedEncodingName
                        + " = "
                        + decoded);

                if (!Arrays.equals(
                        actualBytes,
                        expectedBytes)) {

                    throw new RuntimeException(
                            "FAILED: character_set_results "
                            + "conversion is incorrect."
                            + "\nid                = " + id
                            + "\nexpected encoding = "
                            + expectedEncodingName
                            + "\nserver UTF8       = "
                            + utf8Hex.toUpperCase()
                            + "\nexpected bytes    = "
                            + expectedHex.toUpperCase()
                            + "\nactual JDBC       = "
                            + toHex(actualBytes));
                }

                System.out.println(
                        "PASS: actual JDBC bytes match "
                        + expectedEncodingName);
            }
        }
    }

    private static String querySingleValue(
            Statement stmt,
            String sql) throws Exception {

        try (ResultSet rs = stmt.executeQuery(sql)) {

            if (rs.next())
                return rs.getString(1);
        }

        return null;
    }

    private static void printValue(
            Statement stmt,
            String sql,
            String name) throws Exception {

        try (ResultSet rs = stmt.executeQuery(sql)) {

            if (rs.next()) {
                System.out.println(
                        name + " = " + rs.getString(1));
            }
        }
    }

    private static String toHex(byte[] bytes) {

        if (bytes == null)
            return "NULL";

        StringBuilder sb =
                new StringBuilder();

        for (byte b : bytes) {
            sb.append(
                    String.format(
                            "%02X",
                            b & 0xff));
        }

        return sb.toString();
    }

    private static byte[] hexToBytes(String hex) {

        int len = hex.length();

        byte[] result =
                new byte[len / 2];

        for (int i = 0; i < len; i += 2) {

            result[i / 2] =
                    (byte) Integer.parseInt(
                            hex.substring(i, i + 2),
                            16);
        }

        return result;
    }
}