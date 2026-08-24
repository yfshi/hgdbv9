import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.Statement;
import java.util.Properties;

public class MySQLJdbcBinaryConcatTest
{
    public static void main(String[] args) throws Exception
    {
        if (args.length != 5)
        {
            System.out.println("Usage:");
            System.out.println(
                "java MySQLJdbcBinaryConcatTest " +
                "<host> <port> <dbname> <user> <password>");
            return;
        }

        String url =
            "jdbc:mysql:///?allowPublicKeyRetrieval=true&useSSL=false";

        Properties p = new Properties();
        p.setProperty("HOST", args[0]);
        p.setProperty("PORT", args[1]);
        p.setProperty("DBNAME", args[2]);
        p.setProperty("user", args[3]);
        p.setProperty("password", args[4]);

        try (Connection conn = DriverManager.getConnection(url, p);
             Statement stmt = conn.createStatement())
        {
            /*
             * 1. Create table
             */
            stmt.executeUpdate(
                "DROP TABLE IF EXISTS t_binary_concat"
            );

            stmt.executeUpdate(
                "CREATE TABLE t_binary_concat (" +
                "    id INT PRIMARY KEY," +
                "    c_binary BINARY," +
                "    c_binary3 BINARY(3)," +
                "    c_varbinary VARBINARY(100)," +
                "    c_tinyblob TINYBLOB," +
                "    c_blob BLOB," +
                "    c_mediumblob MEDIUMBLOB," +
                "    c_longblob LONGBLOB," +
                "    c_bit BIT," +
                "    c_bit13 BIT(13)" +
                ")"
            );

            /*
             * 2. Insert test data
             */
            String insertSql =
                "INSERT INTO t_binary_concat (" +
                "    id," +
                "    c_binary," +
                "    c_binary3," +
                "    c_varbinary," +
                "    c_tinyblob," +
                "    c_blob," +
                "    c_mediumblob," +
                "    c_longblob," +
                "    c_bit," +
                "    c_bit13" +
                ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

            try (PreparedStatement ps = conn.prepareStatement(insertSql))
            {
                /*
                 * id = 1
                 *
                 * BINARY    : 58
                 * BINARY(3) : 58 -> 58 00 00
                 *
                 * Binary data:
                 * 41 00 42
                 *
                 * BIT      : 1
                 * BIT(13)  : 1
                 */
                insertRow(
                    ps,
                    1,
                    (byte) 0x58,
                    new byte[] { 0x58 },
                    new byte[] {
                        0x41, 0x00, 0x42
                    },
                    1,
                    0x0001
                );

                /*
                 * id = 2
                 *
                 * Contains 0xFF
                 */
                insertRow(
                    ps,
                    2,
                    (byte) 0x41,
                    new byte[] { 0x41 },
                    new byte[] {
                        0x41, (byte) 0xFF, 0x42
                    },
                    0,
                    0x0000
                );

                /*
                 * id = 3
                 *
                 * Contains backslash 0x5C
                 *
                 * BIT(13):
                 * 1010101010101 = 0x1555
                 */
                insertRow(
                    ps,
                    3,
                    (byte) 0x5C,
                    new byte[] { 0x5C },
                    new byte[] {
                        0x41, 0x5C, 0x42
                    },
                    1,
                    0x1555
                );

                /*
                 * id = 4
                 *
                 * Control bytes:
                 * 00 01 02 0A 0D 1F
                 *
                 * BIT(13):
                 * 1111111111111 = 0x1FFF
                 */
                insertRow(
                    ps,
                    4,
                    (byte) 0x00,
                    new byte[] { 0x00 },
                    new byte[] {
                        0x00,
                        0x01,
                        0x02,
                        0x0A,
                        0x0D,
                        0x1F
                    },
                    0,
                    0x1FFF
                );

                /*
                 * id = 5
                 *
                 * High bytes:
                 * 80 81 FE FF
                 */
                insertRow(
                    ps,
                    5,
                    (byte) 0x7F,
                    new byte[] { 0x7F },
                    new byte[] {
                        (byte) 0x80,
                        (byte) 0x81,
                        (byte) 0xFE,
                        (byte) 0xFF
                    },
                    1,
                    0x0001
                );

                /*
                 * id = 6
                 *
                 * Mixed bytes:
                 * 41 00 5C FF 0A 42
                 *
                 * BIT(13):
                 * 1000000000000 = 0x1000
                 */
                insertRow(
                    ps,
                    6,
                    (byte) 0x42,
                    new byte[] { 0x42 },
                    new byte[] {
                        0x41,
                        0x00,
                        0x5C,
                        (byte) 0xFF,
                        0x0A,
                        0x42
                    },
                    1,
                    0x1000
                );

                /*
                 * id = 7
                 *
                 * UTF-8 "中" + 00 + FF
                 * E4 B8 AD 00 FF
                 *
                 * BIT(13):
                 * 0000000101010 = 0x002A
                 */
                insertRow(
                    ps,
                    7,
                    (byte) 0x43,
                    new byte[] { 0x43 },
                    new byte[] {
                        (byte) 0xE4,
                        (byte) 0xB8,
                        (byte) 0xAD,
                        0x00,
                        (byte) 0xFF
                    },
                    1,
                    0x002A
                );
            }

            /*
             * 3. Query
             *
             * Use HEX() so regression output contains text only.
             * LENGTH() verifies that bytes are not expanded by output
             * conversion such as:
             *
             *   00 -> \000
             *   FF -> \377
             *   5C -> \\
             */
            String sql =
                "SELECT " +
                "    id," +

                "    HEX(c_binary) AS binary_value," +
                "    LENGTH(c_binary) AS binary_len," +
                "    HEX(CONCAT(c_binary, 'y')) AS binary_result," +
                "    LENGTH(CONCAT(c_binary, 'y')) AS binary_concat_len," +

                "    HEX(c_binary3) AS binary3_value," +
                "    LENGTH(c_binary3) AS binary3_len," +
                "    HEX(CONCAT(c_binary3, 'y')) AS binary3_result," +
                "    LENGTH(CONCAT(c_binary3, 'y')) AS binary3_concat_len," +

                "    HEX(CONCAT(c_varbinary, 'y')) AS varbinary_result," +
                "    LENGTH(CONCAT(c_varbinary, 'y')) AS varbinary_len," +

                "    HEX(CONCAT(c_tinyblob, 'y')) AS tinyblob_result," +
                "    LENGTH(CONCAT(c_tinyblob, 'y')) AS tinyblob_len," +

                "    HEX(CONCAT(c_blob, 'y')) AS blob_result," +
                "    LENGTH(CONCAT(c_blob, 'y')) AS blob_len," +

                "    HEX(CONCAT(c_mediumblob, 'y')) AS mediumblob_result," +
                "    LENGTH(CONCAT(c_mediumblob, 'y')) AS mediumblob_len," +

                "    HEX(CONCAT(c_longblob, 'y')) AS longblob_result," +
                "    LENGTH(CONCAT(c_longblob, 'y')) AS longblob_len," +

                "    HEX(c_bit) AS bit_value," +
                "    LENGTH(c_bit) AS bit_len," +
                "    HEX(CONCAT(c_bit, 'y')) AS bit_result," +
                "    LENGTH(CONCAT(c_bit, 'y')) AS bit_concat_len," +

                "    HEX(c_bit13) AS bit13_value," +
                "    LENGTH(c_bit13) AS bit13_len," +
                "    HEX(CONCAT(c_bit13, 'y')) AS bit13_result," +
                "    LENGTH(CONCAT(c_bit13, 'y')) AS bit13_concat_len " +

                "FROM t_binary_concat " +
                "ORDER BY id";

            try (ResultSet rs = stmt.executeQuery(sql))
            {
                System.out.println(
                    "id | " +
                    "BINARY | BINARY_LEN | BINARY_CONCAT | BINARY_CONCAT_LEN | " +
                    "BINARY3 | BINARY3_LEN | BINARY3_CONCAT | BINARY3_CONCAT_LEN | " +
                    "VARBINARY | VLEN | " +
                    "TINYBLOB | TLEN | " +
                    "BLOB | BLEN | " +
                    "MEDIUMBLOB | MLEN | " +
                    "LONGBLOB | LLEN | " +
                    "BIT | BIT_LEN | BIT_CONCAT | BIT_CONCAT_LEN | " +
                    "BIT13 | BIT13_LEN | BIT13_CONCAT | BIT13_CONCAT_LEN"
                );

                while (rs.next())
                {
                    System.out.println(
                        rs.getInt("id") + " | " +

                        rs.getString("binary_value") + " | " +
                        rs.getInt("binary_len") + " | " +
                        rs.getString("binary_result") + " | " +
                        rs.getInt("binary_concat_len") + " | " +

                        rs.getString("binary3_value") + " | " +
                        rs.getInt("binary3_len") + " | " +
                        rs.getString("binary3_result") + " | " +
                        rs.getInt("binary3_concat_len") + " | " +

                        rs.getString("varbinary_result") + " | " +
                        rs.getInt("varbinary_len") + " | " +

                        rs.getString("tinyblob_result") + " | " +
                        rs.getInt("tinyblob_len") + " | " +

                        rs.getString("blob_result") + " | " +
                        rs.getInt("blob_len") + " | " +

                        rs.getString("mediumblob_result") + " | " +
                        rs.getInt("mediumblob_len") + " | " +

                        rs.getString("longblob_result") + " | " +
                        rs.getInt("longblob_len") + " | " +

                        rs.getString("bit_value") + " | " +
                        rs.getInt("bit_len") + " | " +
                        rs.getString("bit_result") + " | " +
                        rs.getInt("bit_concat_len") + " | " +

                        rs.getString("bit13_value") + " | " +
                        rs.getInt("bit13_len") + " | " +
                        rs.getString("bit13_result") + " | " +
                        rs.getInt("bit13_concat_len")
                    );
                }
            }
        }
    }

    private static void insertRow(
        PreparedStatement ps,
        int id,
        byte binaryValue,
        byte[] binary3Value,
        byte[] binaryData,
        int bitValue,
        int bit13Value) throws Exception
    {
        ps.setInt(1, id);

        /*
         * BINARY
         */
        ps.setBytes(2, new byte[] { binaryValue });

        /*
         * BINARY(3)
         *
         * The database should pad trailing 0x00 bytes to length 3.
         */
        ps.setBytes(3, binary3Value);

        /*
         * VARBINARY / BLOB family
         */
        ps.setBytes(4, binaryData);
        ps.setBytes(5, binaryData);
        ps.setBytes(6, binaryData);
        ps.setBytes(7, binaryData);
        ps.setBytes(8, binaryData);

        /*
         * BIT
         */
        ps.setInt(9, bitValue);

        /*
         * BIT(13)
         */
        ps.setInt(10, bit13Value);

        ps.executeUpdate();
    }
}