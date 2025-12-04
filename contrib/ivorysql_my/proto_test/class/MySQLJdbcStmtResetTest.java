import java.sql.*;

public class MySQLJdbcStmtResetTest {
    
    public static void main(String[] args) {
        String host = args[0];
        String port = args[1];
        String database = args[2];
        String username = args[3];
        String password = args[4];
        
        String jdbcUrl = "jdbc:mysql://" + host + ":" + port + "/" + database + "?useSSL=false&useServerPrepStmts=true";
        
        System.out.println("=== COM_STMT_RESET JDBC Test ===\n");
        
        Connection conn = null;
        
        try {
            Class.forName("com.mysql.jdbc.Driver");
            conn = DriverManager.getConnection(jdbcUrl, username, password);
            System.out.println("Database connection successful");
            
            testStatementReset(conn);
            
        } catch (Exception e) {
            System.err.println("Test failed: " + e.getMessage());
            e.printStackTrace();
        } finally {
            try {
                if (conn != null) conn.close();
                System.out.println("\nDatabase connection closed");
            } catch (SQLException e) {
                System.err.println("Resource cleanup failed: " + e.getMessage());
            }
        }
    }
    
    private static void testStatementReset(Connection conn) throws SQLException {
        System.out.println("1. Testing clearParameters on PreparedStatement:");
        
        PreparedStatement pstmt1 = conn.prepareStatement("SELECT $1");
        pstmt1.setInt(1, 100);
        
        pstmt1.clearParameters();
        
        pstmt1.setInt(1, 200);
        ResultSet rs1 = pstmt1.executeQuery();
        if (rs1.next()) {
            System.out.println("After clearParameters and reset: " + rs1.getInt(1));
        }
        rs1.close();
        pstmt1.close();
        
        System.out.println("2. Testing clearBatch on Statement (with DML statements):");
        
        Statement stmt = conn.createStatement();
        
        try {
            stmt.execute("CREATE TABLE IF NOT EXISTS test_batch (id INT, name VARCHAR(50))");
            
            stmt.addBatch("INSERT INTO test_batch VALUES (1, 'test1')");
            stmt.addBatch("INSERT INTO test_batch VALUES (2, 'test2')");
            
            stmt.clearBatch();
            
            stmt.addBatch("INSERT INTO test_batch VALUES (3, 'test3')");
            stmt.addBatch("UPDATE test_batch SET name = 'updated' WHERE id = 3");
            
            int[] results = stmt.executeBatch();
            System.out.println("Batch execution results: " + results.length + " commands executed");
            
            ResultSet rs = stmt.executeQuery("SELECT COUNT(*) FROM test_batch");
            if (rs.next()) {
                System.out.println("Total rows in test_batch: " + rs.getInt(1));
            }
            rs.close();
            
        } finally {
            try {
                stmt.execute("DROP TABLE IF EXISTS test_batch");
            } catch (SQLException e) {
                // Ignore cleanup errors
            }
        }
        
        stmt.close();
        
        System.out.println("3. Testing multiple executions with parameter reset:");
        
        PreparedStatement pstmt3 = conn.prepareStatement("SELECT $1 * 2 AS doubled");
        
        for (int i = 1; i <= 5; i++) {
            pstmt3.setInt(1, i);
            pstmt3.clearParameters();
            pstmt3.setInt(1, i * 10);
            
            ResultSet rs3 = pstmt3.executeQuery();
            if (rs3.next()) {
                System.out.println("Iteration " + i + ": " + (i * 10) + " * 2 = " + rs3.getInt(1));
            }
            rs3.close();
        }
        pstmt3.close();
        
        System.out.println("4. Testing ResultSet cleanup and statement reuse:");
        
        PreparedStatement pstmt4 = conn.prepareStatement("SELECT $1 AS value");
        
        pstmt4.setInt(1, 100);
        ResultSet rs4a = pstmt4.executeQuery();
        if (rs4a.next()) {
            System.out.println("First execution: " + rs4a.getInt(1));
        }
        rs4a.close();
        
        pstmt4.clearParameters();
        pstmt4.setInt(1, 200);
        //pstmt4.setString(1, "200");
        ResultSet rs4b = pstmt4.executeQuery();
        if (rs4b.next()) {
            System.out.println("Second execution after reset: " + rs4b.getInt(1));
        }
        rs4b.close();
        
        pstmt4.close();
        
        Statement tempStmt = conn.createStatement();
        tempStmt.execute("CREATE TABLE IF NOT EXISTS test_batch_temp (id INT, data VARCHAR(50))");
        tempStmt.close();

        System.out.println("5. Testing batch operations with PreparedStatement:");
        
        PreparedStatement pstmt5 = conn.prepareStatement("INSERT INTO test_batch_temp VALUES ($1, $2)");
        
        try {
            for (int i = 1; i <= 3; i++) {    
                pstmt5.setInt(1, i);
                pstmt5.setString(2, "data_" + i);
                pstmt5.addBatch();
            }
            
            pstmt5.clearBatch();
            
            for (int i = 10; i <= 12; i++) {
                pstmt5.setInt(1, i);
                pstmt5.setString(2, "new_data_" + i);
                pstmt5.addBatch();
            }
            
            int[] batchResults = pstmt5.executeBatch();
            System.out.println("PreparedStatement batch executed: " + batchResults.length + " inserts");
            
            Statement verifyStmt = conn.createStatement();
            ResultSet verifyRs = verifyStmt.executeQuery("SELECT COUNT(*) FROM test_batch_temp WHERE id >= 10");
            if (verifyRs.next()) {
                System.out.println("New records inserted: " + verifyRs.getInt(1));
            }
            verifyRs.close();
            verifyStmt.close();
            
        } finally {
            try {
                Statement cleanupStmt = conn.createStatement();
                cleanupStmt.execute("DROP TABLE IF EXISTS test_batch_temp");
                cleanupStmt.close();
            } catch (SQLException e) {
                // Ignore cleanup errors
            }
        }
        
        pstmt5.close();
    }
}