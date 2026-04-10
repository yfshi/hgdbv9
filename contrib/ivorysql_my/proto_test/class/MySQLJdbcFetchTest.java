import java.sql.*;
import java.util.Properties;

/**
 * JDBC example to test MySQL COM_STMT_FETCH packets.
 * Run with a packet capture tool (e.g., Wireshark) or enable MySQL general log
 * to observe the protocol interaction between client and server.
 *
 * Key points:
 * 1. Connection parameter useCursorFetch=true enables cursor support.
 * 2. Set fetchSize on PreparedStatement to a small positive value (e.g., 2),
 *    so the driver uses a server-side cursor and fetches rows in batches.
 *    Each batch triggers a COM_STMT_FETCH request.
 * 3. The query should return more rows than fetchSize to cause multiple FETCHes.
 */
public class MySQLJdbcFetchTest {
    private static String host;
    private static String port = "3306";
    private static String dbname;
    private static String user;
    private static String password;
    private static String jar_version = "old";
    private static String url_jdbc;
    private static Properties info;

    public static void main(String[] args) {
        if (args.length >=5) {
            host = args[0];
            port = args[1];
            dbname = args[2];
            user = args[3];
            password = args[4];
            if (args.length == 6) {
                jar_version = args[5];
            }
        }

        info = new Properties();
        info.setProperty("HOST", host);
        info.setProperty("PORT", port);
        info.setProperty("DBNAME", dbname);
        info.setProperty("user", user);
        info.setProperty("password", password);

        if (jar_version.equals("new")) {
            url_jdbc = "jdbc:mysql://";
        } else {
            url_jdbc = "jdbc:mysql://?allowPublicKeyRetrieval=true&useSSL=false";
        }


        // Initialize test table and insert data
        initTestData();

        // Test cursor-based batch fetching
        testCursorFetch();
    }

    /**
     * Create test table and insert 10 records.
     */
    private static void initTestData() {
        String createTableSQL = "CREATE TABLE IF NOT EXISTS test_fetch (" +
                "id INT PRIMARY KEY AUTO_INCREMENT, " +
                "name VARCHAR(50), " +
                "age VARCHAR(50))";
        String insertSQL = "INSERT INTO test_fetch (name, age) VALUES (?, ?)";

        try (Connection conn = DriverManager.getConnection(url_jdbc, info);
             Statement stmt = conn.createStatement()) {

            // Create table
            stmt.executeUpdate(createTableSQL);

            // Clear old data (optional)
            stmt.executeUpdate("truncate test_fetch");

            // Insert 10 test records
            try (PreparedStatement pstmt = conn.prepareStatement(insertSQL)) {
                for (int i = 1; i <= 10; i++) {
                    pstmt.setString(1, "Name" + i);
                    pstmt.setString(2, "Age" + i);
                    pstmt.executeUpdate();
                }

                pstmt.setString(1, null);
                pstmt.setString(2, null);
                pstmt.executeUpdate();
            }
            System.out.println("Test table initialized, inserted 10 rows.\n");

        } catch (SQLException e) {
            e.printStackTrace();
        }
    }

    /**
     * Execute query with fetchSize = 2 and observe batch fetching behavior.
     * Each call to ResultSet.next() may trigger a new batch fetch from the server,
     * which sends COM_STMT_FETCH packets.
     */
    private static void testCursorFetch() {
        String querySQL = "SELECT name, id, age FROM test_fetch ORDER BY id";

        try (Connection conn = DriverManager.getConnection(url_jdbc, info);
             PreparedStatement pstmt = conn.prepareStatement(querySQL)) {

            // Set the number of rows to fetch from the server per batch (must be > 0 to enable cursor)
            pstmt.setFetchSize(2);

            System.out.println("Starting query execution, fetchSize = 2");
            try (ResultSet rs = pstmt.executeQuery()) {

                int rowCount = 0;
                while (rs.next()) {
                    int id = rs.getInt("id");
                    String name = rs.getString("name");
                    String age = rs.getString("age");
                    System.out.printf("Reading row %d: id=%d, name=%s, age=%s%n", ++rowCount, id, name, age);

                    // Optional: pause after each row to ease packet capture observation
                    // Thread.sleep(100);
                }
                System.out.println("Total rows read: " + rowCount + ".");
            } catch (SQLException e) {
                e.printStackTrace();
            }

        } catch (SQLException e) {
            e.printStackTrace();
        }
    }
}