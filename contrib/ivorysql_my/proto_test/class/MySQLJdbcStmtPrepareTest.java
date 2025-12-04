import java.sql.*;

public class MySQLJdbcStmtPrepareTest {
    
    public static void main(String[] args) {
        String host = args[0];
        String port = args[1];
        String database = args[2];
        String username = args[3];
        String password = args[4];
        
        String jdbcUrl = "jdbc:mysql://" + host + ":" + port + "/" + database + "?useSSL=false&useServerPrepStmts=true";
        
        System.out.println("=== COM_STMT_PREPARE JDBC Test ===\n");
        
        Connection conn = null;
        
        try {
            Class.forName("com.mysql.jdbc.Driver");
            conn = DriverManager.getConnection(jdbcUrl, username, password);
            System.out.println("Database connection successful");
            
            testPreparedStatements(conn);
            
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
    
    private static void testPreparedStatements(Connection conn) throws SQLException {
        PreparedStatement pstmt1 = conn.prepareStatement("SELECT $1 + $2");
        pstmt1.setInt(1, 10);
        pstmt1.setInt(2, 20);
        
        ResultSet rs1 = pstmt1.executeQuery();
        if (rs1.next()) {
            System.out.println("Prepared statement result: " + rs1.getInt(1));
        }
        rs1.close();
        pstmt1.close();
        
        /*
        PreparedStatement pstmt2 = conn.prepareStatement("SELECT CONCAT($1, $2)");
        pstmt2.setString(1, "Hello, ");
        pstmt2.setString(2, "World!");
        
        ResultSet rs2 = pstmt2.executeQuery();
        if (rs2.next()) {
            System.out.println("String concatenation result: " + rs2.getString(1));
        }
        rs2.close();
        pstmt2.close();
        */
        
        PreparedStatement pstmt3 = conn.prepareStatement("SELECT $1, $2, $3");
        pstmt3.setString(1, "100");
        pstmt3.setString(2, "test");
        pstmt3.setBoolean(3, true);
        
        ResultSet rs3 = pstmt3.executeQuery();
        if (rs3.next()) {
            System.out.println("Multiple parameters: " + 
                rs3.getInt(1) + ", " + 
                rs3.getString(2) + ", " + 
                rs3.getBoolean(3));
        }
        rs3.close();
        pstmt3.close();
        
    }
}