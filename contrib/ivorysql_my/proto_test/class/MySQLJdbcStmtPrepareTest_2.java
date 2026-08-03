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
            stmt.executeUpdate("DROP TABLE IF EXISTS test_2");
            stmt.executeUpdate("CREATE TABLE test_2 (c INT, d INT)");
            stmt.executeUpdate("INSERT INTO test_2 VALUES (10, 120),(15, 100),(30, 80)");
            stmt.executeUpdate("DROP TABLE IF EXISTS test_3");
            stmt.executeUpdate("CREATE TABLE test_3 (id INT auto_increment primary key, isvalid INT default 0)");
            stmt.executeUpdate("INSERT INTO test_3 (isvalid) VALUES (100), (120), (200), (90)");
            stmt.executeUpdate("DROP TABLE IF EXISTS test_4");
            stmt.executeUpdate("CREATE TABLE test_4 (a INT, b TEXT, c INT)");
            stmt.executeUpdate("INSERT INTO test_4 VALUES (1000, 'StmtPrepare TEST', 50), (600, 'HIGH GO', 50), (2000, 'OrderBy TEST', 50), (700, 'MYS_MODE', 30)");

            testPreparedStatementWithoutParams(conn);
            testPreparedStatementWithParam(conn);
            testPreparedStatementWithOrderBy_1(conn);
            testPreparedStatementWithOrderBy_2(conn);
            testPreparedStatementWithOrderBy_3(conn);
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

    private static void testPreparedStatementWithOrderBy_1(Connection conn) {
        PreparedStatement pstmt = null;
        ResultSet rs = null;

        try {
            System.out.println("\n--- Test 3-1: PreparedStatement WITHOUT parameters contains order by ---");
            pstmt = conn.prepareStatement("SELECT c FROM test_2 order by d");
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
                System.out.println("No data found in table test_3.");
            }
        } catch (SQLException e) {
            System.err.println("Failed to execute query without parameter contains order by: " + e.getMessage());
        } finally {
            try {
                if (rs != null) rs.close();
                if (pstmt != null) pstmt.close();
            } catch (SQLException e) {
                System.err.println("Cleanup error: " + e.getMessage());
            }
        }
    }

    private static void testPreparedStatementWithOrderBy_2(Connection conn) {
        PreparedStatement pstmt = null;
        ResultSet rs = null;

        try {
            System.out.println("\n--- Test 3-2: PreparedStatement WITHOUT parameters contains order by ---");
            pstmt = conn.prepareStatement("SELECT isvalid FROM test_3 order by id");
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
                System.out.println("No data found in table test_3.");
            }
        } catch (SQLException e) {
            System.err.println("Failed to execute query without parameter contains order by: " + e.getMessage());
        } finally {
            try {
                if (rs != null) rs.close();
                if (pstmt != null) pstmt.close();
            } catch (SQLException e) {
                System.err.println("Cleanup error: " + e.getMessage());
            }
        }
    }

    private static void testPreparedStatementWithOrderBy_3(Connection conn) {
        PreparedStatement pstmt = null;
        ResultSet rs = null;

        try {
            System.out.println("\n--- Test 3-3: PreparedStatement WITH parameter contains parameter ---");
            pstmt = conn.prepareStatement("SELECT b FROM test_4 WHERE c =? order by a");

            pstmt.setInt(1, 50);
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
                System.out.println("No record found with c = 50");
            }

        } catch (SQLException e) {
            System.err.println("Failed to execute query with parameter contains order by: " + e.getMessage());
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