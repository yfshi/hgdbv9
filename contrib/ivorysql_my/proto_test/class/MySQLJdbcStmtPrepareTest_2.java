import java.sql.*;
import java.util.Properties;

public class MySQLJdbcStmtPrepareTest_2 {
    private static String host;
	private static String port = "3307";
	private static String dbname;
	private static String user;
	private static String password;
	private static String jar_version = "old";
	private static String url_jdbc;

	public static void main(String[] args) throws Exception {
		if (args.length >= 4) {
			host = args[0];
			port = args[1];
			dbname = args[2];
			user = args[3];
		}

		Properties info = new Properties();
		info.setProperty("HOST", host);
		info.setProperty("PORT", port);
		info.setProperty("DBNAME", dbname);
		info.setProperty("user", user);

		if (jar_version.equals("new"))
		{
			url_jdbc = "jdbc:mysql://?allowPublicKeyRetrieval=true&useServerPrepStmts=true&serverTimezone=UTC";
		}
		else
		{
			url_jdbc = "jdbc:mysql://?allowPublicKeyRetrieval=true&useServerPrepStmts=true&serverTimezone=UTC&useSSL=false";
		}

		Connection conn = null;
		Statement stmt = null;

		try {
            Class.forName("com.mysql.jdbc.Driver");
            conn = DriverManager.getConnection(url_jdbc, info);
            System.out.println("Database connection successful");

            stmt = conn.createStatement();
            stmt.executeUpdate("DROP TABLE IF EXISTS test_1");
            stmt.executeUpdate("CREATE TABLE test_1 (a INT, b TEXT)");
            stmt.executeUpdate("INSERT INTO test_1 VALUES (1, 'ABC')");

            testPreparedStatementWithoutParams(conn);
            testPreparedStatementWithParam(conn);
        } catch (Exception e) {
            System.err.println("Test failed: " + e.getMessage());
            e.printStackTrace();
        } finally {
            try {
                if (stmt != null) stmt.close();
                if (conn != null) conn.close();
                System.out.println("\nDatabase connection closed");
            } catch (SQLException e) {
                System.err.println("Resource cleanup failed: " + e.getMessage());
            }
        }
	}

    private static void testPreparedStatementWithoutParams(Connection conn) {
        PreparedStatement pstmt = null;
        ResultSet rs = null;

        try {
            System.out.println("--- Test 1: PreparedStatement WITHOUT parameters ---");
            pstmt = conn.prepareStatement("SELECT * FROM test_1");
            rs = pstmt.executeQuery();
            
            ResultSetMetaData rsmd = rs.getMetaData();
            int columnCount = rsmd.getColumnCount();

            System.out.print("Columns: ");
            for (int i = 1; i <= columnCount; i++) {
                System.out.print(rsmd.getColumnName(i));
                if (i < columnCount) System.out.print(", ");
            }
            System.out.println();

            int rowNum = 0;
            while (rs.next()) {
                rowNum++;
                System.out.print("Row " + rowNum + ": ");
                for (int i = 1; i <= columnCount; i++) {
                    System.out.print(rsmd.getColumnName(i) + "=" + rs.getString(i));
                    if (i < columnCount) System.out.print(", ");
                }
                System.out.println();
            }

            if (rowNum == 0) {
                System.out.println("No data found in table test_1.");
            }
        } catch (SQLException e) {
            System.err.println("Failed to execute query without parameters: " + e.getMessage());
        } finally {
            try {
                if (rs != null) rs.close();
                if (pstmt != null) pstmt.close();
            } catch (SQLException e) {
                System.err.println("Cleanup error: " + e.getMessage());
            }
        }
    }

    private static void testPreparedStatementWithParam(Connection conn) {
        PreparedStatement pstmt = null;
        ResultSet rs = null;

        try {
            System.out.println("\n--- Test 2: PreparedStatement WITH parameter ---");
            pstmt = conn.prepareStatement("SELECT * FROM test_1 WHERE a =?");

            pstmt.setInt(1, 1);
            rs = pstmt.executeQuery();

            ResultSetMetaData rsmd = rs.getMetaData();
            int columnCount = rsmd.getColumnCount();

            System.out.print("Columns: ");
            for (int i = 1; i <= columnCount; i++) {
                System.out.print(rsmd.getColumnName(i));
                if (i < columnCount) System.out.print(", ");
            }
            System.out.println();

            int rowNum = 0;
            while (rs.next()) {
                rowNum++;
                System.out.print("Row " + rowNum + ": ");
                for (int i = 1; i <= columnCount; i++) {
                    System.out.print(rsmd.getColumnName(i) + "=" + rs.getString(i));
                    if (i < columnCount) System.out.print(", ");
                }
                System.out.println();
            }

            if (rowNum == 0) {
                System.out.println("No record found with a = 1");
            }

        } catch (SQLException e) {
            System.err.println("Failed to execute query with parameter: " + e.getMessage());
        } finally {
            try {
                if (rs != null) rs.close();
                if (pstmt != null) pstmt.close();
            } catch (SQLException e) {
                System.err.println("Cleanup error: " + e.getMessage());
            }
        }
    }
}