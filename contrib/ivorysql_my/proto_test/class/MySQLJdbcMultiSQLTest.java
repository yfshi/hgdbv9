import java.sql.*;
import java.util.Arrays;

/**
 * JDBC Multi-Statement Test Case
 * Demonstrates combined execution of DDL and DML statements with allowMultiQueries=true
 */
public class MySQLJdbcMultiSQLTest {

    // Database connection configuration
    private static String HOST;
    private static String PORT;
    private static String DATABASE;
    private static String USER;
    private static String PASSWORD;

    public static void main(String[] args) {
        if (args.length >=5) {
            HOST = args[0];
            PORT = args[1];
            DATABASE = args[2];
            USER = args[3];
            PASSWORD = args[4];
        }

        // Connection URL with multi-statement support enabled
        String url = String.format(
            "jdbc:mysql://%s:%s/%s?useSSL=false&allowMultiQueries=true&serverTimezone=UTC",
            HOST, PORT, DATABASE, USER, PASSWORD
        );

        Connection conn = null;
        Statement stmt = null;

        try {
            // 1. Establish connection
            conn = DriverManager.getConnection(url, USER, PASSWORD);
            stmt = conn.createStatement();

            System.out.println("========== Test Case 1: DDL + DML Mixed Execution ==========");

            // 2. Build multi-statement SQL containing DDL and DML
            String multiSql = 
                // DDL: Drop table if exists
                "DROP TABLE IF EXISTS test_multi_queries; " +
                
                // DDL: Create table
                "CREATE TABLE test_multi_queries (" +
                "    id INT PRIMARY KEY AUTO_INCREMENT, " +
                "    name VARCHAR(50) NOT NULL, " +
                "    age INT" +
                "); " +
                
                // DML: Insert data
                "INSERT INTO test_multi_queries (name, age) VALUES ('Zhang San', 25); " +
                "INSERT INTO test_multi_queries (name, age) VALUES ('Li Si', 30); " +
                "INSERT INTO test_multi_queries (name, age) VALUES ('Wang Wu', 28); " +
                
                // DML: Update data
                "UPDATE test_multi_queries SET age = 26 WHERE name = 'Zhang San'; " +
                
                // DML: Delete data
                "DELETE FROM test_multi_queries WHERE name = 'Wang Wu'; " +
                
                // DQL: Query data
                "SELECT * FROM test_multi_queries ORDER BY id; " +
                
                // DQL: Aggregate query
                "SELECT COUNT(*) as total_count, AVG(age) as avg_age FROM test_multi_queries;";

            System.out.println("Executing multi-statement SQL:\n" + multiSql);
            System.out.println("\nStarting multi-statement execution...");

            // 3. Execute multi-statement
            boolean isResultSet = stmt.execute(multiSql);
            int statementCount = 1;

            // 4. Iterate through multiple results
            do {
                if (isResultSet) {
                    // Handle query results (SELECT statements)
                    ResultSet rs = stmt.getResultSet();
                    System.out.println("\n--- Result Set #" + statementCount + " (Query Result) ---");
                    printResultSet(rs);
                    rs.close();
                } else {
                    // Handle update counts (INSERT/UPDATE/DELETE/DDL statements)
                    int updateCount = stmt.getUpdateCount();
                    if (updateCount == -1) {
                        // No more results
                        break;
                    }
                    System.out.println("\n--- Result #" + statementCount + " (Update Count) ---");
                    System.out.println("Rows affected: " + updateCount);
                    
                    // DDL statements (CREATE/DROP/ALTER) typically return 0
                    if (updateCount == 0) {
                        System.out.println("(This is a DDL statement or DML with no affected rows)");
                    }
                }
                
                // Move to the next result
                isResultSet = stmt.getMoreResults();
                statementCount++;
                
            } while (true);
        } catch (SQLException e) {
            System.err.println("SQL Execution Exception: " + e.getMessage());
            System.err.println("Error Code: " + e.getErrorCode());
            System.err.println("SQL State: " + e.getSQLState());
            e.printStackTrace();
        } finally {
            // 5. Close resources
            try {
                if (stmt != null) stmt.close();
                if (conn != null) conn.close();
            } catch (SQLException e) {
                e.printStackTrace();
            }
        }
    }

    /**
     * Print ResultSet contents in tabular format
     */
    private static void printResultSet(ResultSet rs) throws SQLException {
        ResultSetMetaData metaData = rs.getMetaData();
        int columnCount = metaData.getColumnCount();

        // Print column headers
        for (int i = 1; i <= columnCount; i++) {
            System.out.print(metaData.getColumnName(i) + "\t");
        }
        System.out.println();
        System.out.println("----------------------------------------");

        // Print data rows
        int rowCount = 0;
        while (rs.next()) {
            for (int i = 1; i <= columnCount; i++) {
                System.out.print(rs.getString(i) + "\t");
            }
            System.out.println();
            rowCount++;
        }
        System.out.println("Total rows: " + rowCount);
    }
}