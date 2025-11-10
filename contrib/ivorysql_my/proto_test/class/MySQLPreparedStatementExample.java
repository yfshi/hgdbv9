import java.io.*;
import java.sql.*;
import java.util.Properties;

public class MySQLPreparedStatementExample {
    private static String host;
    private static String port = "3307";
    private static String dbname;
    private static String user;
    private static String password;
    private static String jar_version = "old";
    private static String URL;
    
    public static void main(String[] args) {
        Connection conn = null;

        if (args.length >= 5) {
            host = args[0];
            port = args[1];
            dbname = args[2];
            user = args[3];
            password = args[4];
            if (args.length == 6) {
                jar_version = args[5];
            }
        }
        
        try {
            // 1. Load driver and establish connection
            Class.forName("com.mysql.jdbc.Driver");
            
            Properties props = new Properties();

            props.setProperty("HOST", host);
            props.setProperty("PORT", port);
            props.setProperty("DBNAME", dbname);
            props.setProperty("user", user);
            props.setProperty("password", password);

            if (jar_version.equals("new")) {
                URL = "jdbc:mysql://?allowPublicKeyRetrieval=true&useServerPrepStmts=true&serverTimezone=UTC";
            } else {
                URL = "jdbc:mysql://?allowPublicKeyRetrieval=true&useServerPrepStmts=true&serverTimezone=UTC&useSSL=false";
            }
            
            // Enable server-side prepared statements
            props.setProperty("useServerPrepStmts", "true");
            // Enable statement pooling to demonstrate COM_STMT_CLOSE
            props.setProperty("cachePrepStmts", "true");
            props.setProperty("prepStmtCacheSize", "25");
            props.setProperty("prepStmtCacheSqlLimit", "2048");
            // Set chunk size for long data
            props.setProperty("blobSendChunkSize", "16384");
            
            conn = DriverManager.getConnection(URL, props);
            
            // 2. Create table if not exists
            createTable(conn);
            
            // 3. Demonstrate COM_STMT_SEND_LONG_DATA with text data
            insertLongTextData(conn);
            
            // 4. Demonstrate COM_STMT_SEND_LONG_DATA with binary data
            insertLongBinaryData(conn);
            
            // 5. Demonstrate COM_STMT_RESET
            demonstrateStmtReset(conn);
            
            // 6. Demonstrate COM_STMT_CLOSE
            demonstrateStmtClose(conn);
            
            // 7. Read and verify data
            readData(conn);
            
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            // Close resources
            try {
                if (conn != null) conn.close();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }
    
    /**
     * Create test table
     */
    private static void createTable(Connection conn) throws SQLException {
        String createTableSQL = 
            "CREATE TABLE IF NOT EXISTS long_data_test (" +
            "id INT AUTO_INCREMENT PRIMARY KEY, " +
            "file_name VARCHAR(255), " +
            "large_text LONGTEXT, " +
            "binary_data LONGBLOB, " +
            "description VARCHAR(500), " +
            "created_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP" +
            ")";
        
        try (Statement stmt = conn.createStatement()) {
            stmt.execute("drop table if exists long_data_test");
            stmt.execute(createTableSQL);
            System.out.println("Table created successfully or already exists");
        }
    }
    
    /**
     * Insert long text data - uses COM_STMT_SEND_LONG_DATA
     */
    private static void insertLongTextData(Connection conn) throws SQLException, IOException {
        String insertSQL = "INSERT INTO long_data_test (file_name, large_text, description) VALUES ($1, $2, $3)";
        
        try (PreparedStatement pstmt = conn.prepareStatement(insertSQL)) {
            pstmt.setString(1, "large_text_file.txt");
            pstmt.setString(3, "COM_STMT_SEND_LONG_DATA example with text data");
            
            // Create large text content
            StringBuilder largeText = new StringBuilder();
            for (int i = 0; i < 5000; i++) {
                largeText.append("This is line ").append(i).append(" of text data. ");
                largeText.append("When text data is large, JDBC driver uses COM_STMT_SEND_LONG_DATA for chunked transfer.\n");
            }
            
            // Using setCharacterStream triggers COM_STMT_SEND_LONG_DATA
            StringReader reader = new StringReader(largeText.toString());
            pstmt.setCharacterStream(2, reader, largeText.length());
            
            int rows = pstmt.executeUpdate();
            System.out.println("Inserted long text data successfully, affected rows: " + rows);
        }
    }
    
    /**
     * Insert long binary data - uses COM_STMT_SEND_LONG_DATA
     */
    private static void insertLongBinaryData(Connection conn) throws SQLException, IOException {
        String insertSQL = "INSERT INTO long_data_test (file_name, binary_data, description) VALUES ($1, $2, $3)";
        
        try (PreparedStatement pstmt = conn.prepareStatement(insertSQL)) {
            pstmt.setString(1, "large_binary_file.dat");
            pstmt.setString(3, "COM_STMT_SEND_LONG_DATA example with binary data");
            
            // Create large binary data (simulating large file)
            byte[] largeBinaryData = createLargeBinaryData(1024 * 1024); // 1MB
            
            // Using setBinaryStream triggers COM_STMT_SEND_LONG_DATA
            ByteArrayInputStream bis = new ByteArrayInputStream(largeBinaryData);
            pstmt.setBinaryStream(2, bis, largeBinaryData.length);
            
            int rows = pstmt.executeUpdate();
            System.out.println("Inserted long binary data successfully, affected rows: " + rows);
        }
    }
    
    /**
     * Demonstrate COM_STMT_RESET - Reset prepared statement parameters
     */
    private static void demonstrateStmtReset(Connection conn) throws SQLException, IOException {
        String insertSQL = "INSERT INTO long_data_test (file_name, binary_data, description) VALUES ($1, $2, $3)";
        
        try (PreparedStatement pstmt = conn.prepareStatement(insertSQL)) {
            
            System.out.println("\n=== Demonstrating COM_STMT_RESET ===");
            
            // First execution
            pstmt.setString(1, "reset_example_1.dat");
            pstmt.setString(3, "First execution before reset");
            byte[] data1 = createLargeBinaryData(512 * 1024); // 512KB
            ByteArrayInputStream bis1 = new ByteArrayInputStream(data1);
            pstmt.setBinaryStream(2, bis1, data1.length);
            
            int rows1 = pstmt.executeUpdate();
            System.out.println("First execution - affected rows: " + rows1);
            
            // COM_STMT_RESET occurs internally when we reuse the prepared statement
            // with clearParameters() or when setting new parameters
            
            // Clear parameters (triggers COM_STMT_RESET internally in some drivers)
            pstmt.clearParameters();
            
            // Second execution with different parameters
            pstmt.setString(1, "reset_example_2.dat");
            pstmt.setString(3, "Second execution after reset");
            byte[] data2 = createLargeBinaryData(256 * 1024); // 256KB
            ByteArrayInputStream bis2 = new ByteArrayInputStream(data2);
            pstmt.setBinaryStream(2, bis2, data2.length);
            
            int rows2 = pstmt.executeUpdate();
            System.out.println("Second execution after reset - affected rows: " + rows2);
            
            // Another way to trigger COM_STMT_RESET - execute batch operations
            demonstrateBatchWithReset(conn);
        }
    }
    
    /**
     * Demonstrate batch operations that use COM_STMT_RESET
     */
    private static void demonstrateBatchWithReset(Connection conn) throws SQLException, IOException {
        String insertSQL = "INSERT INTO long_data_test (file_name, description, large_text) VALUES ($1, $2, $3)";
        
        try (PreparedStatement pstmt = conn.prepareStatement(insertSQL)) {
            System.out.println("\n=== Demonstrating COM_STMT_RESET with Batch ===");
            
            // Add multiple batches
            for (int i = 1; i <= 3; i++) {
                pstmt.setString(1, "batch_file_" + i + ".txt");
                pstmt.setString(2, "Batch example " + i + " - COM_STMT_RESET between batches");
                
                // Create different text data for each batch
                StringBuilder text = new StringBuilder();
                for (int j = 0; j < 1000; j++) {
                    text.append("Batch ").append(i).append(", line ").append(j).append(": Data for COM_STMT_RESET demonstration.\n");
                }
                
                StringReader reader = new StringReader(text.toString());
                pstmt.setCharacterStream(3, reader, text.length());
                pstmt.addBatch();
                
                // COM_STMT_RESET occurs internally between batch executions
            }
            
            int[] results = pstmt.executeBatch();
            System.out.println("Batch execution completed. Results: " + java.util.Arrays.toString(results));
        }
    }
    
    /**
     * Demonstrate COM_STMT_CLOSE - Closing prepared statements
     */
    private static void demonstrateStmtClose(Connection conn) throws SQLException {
        System.out.println("\n=== Demonstrating COM_STMT_CLOSE ===");
        
        // Method 1: Explicit close in try-with-resources (triggers COM_STMT_CLOSE)
        System.out.println("Method 1: Using try-with-resources for automatic COM_STMT_CLOSE");
        for (int i = 0; i < 3; i++) {
            String sql = "SELECT id, file_name FROM long_data_test WHERE id < $1 order by id";
            try (PreparedStatement pstmt = conn.prepareStatement(sql)) {
                pstmt.setInt(1, (i + 1) * 10);
                try (ResultSet rs = pstmt.executeQuery()) {
                    int count = 0;
                    while (rs.next()) {
                        count++;
                    }
                    System.out.println("Query " + (i + 1) + " returned " + count + " rows");
                }
            } // COM_STMT_CLOSE is sent here when pstmt is closed
        }
        
        // Method 2: Explicit close() method call
        System.out.println("Method 2: Using explicit close() for COM_STMT_CLOSE");
        PreparedStatement pstmt1 = null;
        PreparedStatement pstmt2 = null;
        
        try {
            pstmt1 = conn.prepareStatement("SELECT COUNT(*) FROM long_data_test");
            try (ResultSet rs = pstmt1.executeQuery()) {
                if (rs.next()) {
                    System.out.println("Total records: " + rs.getInt(1));
                }
            }
            
            pstmt2 = conn.prepareStatement("SELECT file_name FROM long_data_test WHERE description LIKE $1 order by 1");
            pstmt2.setString(1, "%COM_STMT%");
            try (ResultSet rs = pstmt2.executeQuery()) {
                int count = 0;
                while (rs.next()) {
                    count++;
                }
                System.out.println("Found " + count + " records with COM_STMT in description");
            }
        } finally {
            // Explicitly close statements to trigger COM_STMT_CLOSE
            if (pstmt1 != null) {
                pstmt1.close(); // Sends COM_STMT_CLOSE
                System.out.println("First prepared statement closed");
            }
            if (pstmt2 != null) {
                pstmt2.close(); // Sends COM_STMT_CLOSE
                System.out.println("Second prepared statement closed");
            }
        }
        
        // Method 3: Demonstrate that statements are automatically closed when connection closes
        demonstrateConnectionClose();
    }
    
    /**
     * Demonstrate that all statements are closed when connection closes
     */
    private static void demonstrateConnectionClose() {
        System.out.println("\n=== Demonstrating COM_STMT_CLOSE on Connection Close ===");
        
        Connection tempConn = null;
        try {
            Properties props = new Properties();
            props.setProperty("HOST", host);
            props.setProperty("PORT", port);
            props.setProperty("DBNAME", dbname);
            props.setProperty("user", user);
            props.setProperty("password", password);

            if (jar_version.equals("new")) {
                URL = "jdbc:mysql://?allowPublicKeyRetrieval=true&useServerPrepStmts=true&serverTimezone=UTC";
            } else {
                URL = "jdbc:mysql://?allowPublicKeyRetrieval=true&useServerPrepStmts=true&serverTimezone=UTC&useSSL=false";
            }
            
            props.setProperty("useServerPrepStmts", "true");
            props.setProperty("cachePrepStmts", "true");
            props.setProperty("prepStmtCacheSize", "25");
            props.setProperty("prepStmtCacheSqlLimit", "2048");
            // Set chunk size for long data
            props.setProperty("blobSendChunkSize", "16384");
            
            tempConn = DriverManager.getConnection(URL, props);
            
            // Create multiple prepared statements
            PreparedStatement pstmt1 = tempConn.prepareStatement("SELECT 1");
            PreparedStatement pstmt2 = tempConn.prepareStatement("SELECT 2");
            PreparedStatement pstmt3 = tempConn.prepareStatement("SELECT 3");
            
            System.out.println("Created 3 prepared statements");
            System.out.println("When connection closes, COM_STMT_CLOSE will be sent for all open statements");
            
        } catch (SQLException e) {
            e.printStackTrace();
        } finally {
            if (tempConn != null) {
                try {
                    tempConn.close(); // This sends COM_STMT_CLOSE for all open statements
                    System.out.println("Connection closed - all statements received COM_STMT_CLOSE");
                } catch (SQLException e) {
                    e.printStackTrace();
                }
            }
        }
    }
    
    /**
     * Read and verify data
     */
    private static void readData(Connection conn) throws SQLException {
        String querySQL = "SELECT id, file_name, LENGTH(large_text) as text_length, " +
                         "LENGTH(binary_data) as binary_length, description, created_time " +
                         "FROM long_data_test ORDER BY id";
        
        try (Statement stmt = conn.createStatement();
             ResultSet rs = stmt.executeQuery(querySQL)) {
            
            System.out.println("\n=== Data Verification Results ===");
            System.out.printf("%-5s %-25s %-12s %-12s %-50s%n", 
                "ID", "File Name", "Text Length", "Bin Length", "Description");
            System.out.println("----------------------------------------------------------------------------------------------------");
            
            while (rs.next()) {
                int id = rs.getInt("id");
                String fileName = rs.getString("file_name");
                long textLength = rs.getLong("text_length");
                long binaryLength = rs.getLong("binary_length");
                String description = rs.getString("description");
                
                System.out.printf("%-5d %-25s %-12d %-12d %-50s%n", 
                    id, fileName, textLength, binaryLength, 
                    description.length() > 50 ? description.substring(0, 47) + "..." : description);
            }
        }
    }
    
    /**
     * Create simulated large binary data
     */
    private static byte[] createLargeBinaryData(int size) {
        byte[] data = new byte[size];
        // Fill with pattern data for verification
        for (int i = 0; i < size; i++) {
            data[i] = (byte) (i % 256);
        }
        return data;
    }
}