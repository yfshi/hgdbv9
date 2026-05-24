import java.sql.*;
import java.util.Properties;

public class MySQLJdbcTimeDecimalTest {
    private static String host;
    private static String port = "3306";
    private static String dbname;
    private static String user;
    private static String password;
    private static Properties info;

    public static void main(String[] args) throws Exception {
        if (args.length >= 5) {
            host     = args[0];
            port     = args[1];
            dbname   = args[2];
            user     = args[3];
            password = args[4];
        }

        info = new Properties();
        info.setProperty("HOST",     host);
        info.setProperty("PORT",     port);
        info.setProperty("DBNAME",   dbname);
        info.setProperty("user",     user);
        info.setProperty("password", password);

        String baseUrl  = "jdbc:mysql://?allowPublicKeyRetrieval=true&useSSL=false&serverTimezone=UTC";
        String simpleUrl = baseUrl + "&preferQueryMode=simple";

        System.out.println("=== preferQueryMode=simple ===");
        runTest(simpleUrl);

        System.out.println("=== executeBatch ===");
        runBatch(baseUrl);
    }

    static void runTest(String url) throws Exception {
        try (Connection conn = DriverManager.getConnection(url, info);
             Statement st = conn.createStatement()) {
            st.execute("DROP TABLE IF EXISTS test_tdec");
            st.execute("CREATE TABLE test_tdec (t TIME(6), dt DATETIME(3), ts TIMESTAMP(4) NULL)");

            PreparedStatement ps = conn.prepareStatement("INSERT INTO test_tdec VALUES (?, ?, ?)");
            ps.setString(1, "12:34:56.789012");
            ps.setString(2, "2024-01-15 12:34:56.123");
            ps.setString(3, "2024-06-01 08:00:00.1234");
            ps.executeUpdate();
            ps.close();

            ResultSet rs = st.executeQuery("SELECT t, dt, ts FROM test_tdec");
            rs.next();
            System.out.println("TIME(6)      write:12:34:56.789012           read:" + rs.getString("t"));
            System.out.println("DATETIME(3)  write:2024-01-15 12:34:56.123   read:" + rs.getString("dt"));
            System.out.println("TIMESTAMP(4) write:2024-06-01 08:00:00.1234  read:" + rs.getString("ts"));
            rs.close();
            st.execute("DROP TABLE test_tdec");
        }
    }

    static void runBatch(String url) throws Exception {
        try (Connection conn = DriverManager.getConnection(url, info);
             Statement st = conn.createStatement()) {
            st.execute("DROP TABLE IF EXISTS test_tdec_batch");
            st.execute("CREATE TABLE test_tdec_batch (t TIME(6), dt DATETIME(3), ts TIMESTAMP(4) NULL)");

            PreparedStatement ps = conn.prepareStatement("INSERT INTO test_tdec_batch VALUES (?, ?, ?)");
            ps.setString(1, "12:34:56.789012");
            ps.setString(2, "2024-01-15 12:34:56.123");
            ps.setString(3, "2024-06-01 08:00:00.1234");
            ps.addBatch();
            ps.executeBatch();
            ps.close();

            ResultSet rs = st.executeQuery("SELECT t, dt, ts FROM test_tdec_batch");
            rs.next();
            System.out.println("TIME(6)      write:12:34:56.789012           read:" + rs.getString("t"));
            System.out.println("DATETIME(3)  write:2024-01-15 12:34:56.123   read:" + rs.getString("dt"));
            System.out.println("TIMESTAMP(4) write:2024-06-01 08:00:00.1234  read:" + rs.getString("ts"));
            rs.close();
            st.execute("DROP TABLE test_tdec_batch");
        }
    }
}
