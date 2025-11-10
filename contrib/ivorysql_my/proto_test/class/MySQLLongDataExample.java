import java.io.*;
import java.sql.*;
import java.util.Properties;

public class MySQLLongDataExample {
    private static String host;
    private static String port = "3307";
    private static String dbname;
    private static String user;
    private static String password;
    private static String jar_version = "old";
    private static String URL;
    
    public static void main(String[] args) {
        Connection conn = null;
        PreparedStatement pstmt = null;
        FileInputStream fis = null;

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
            Class.forName("com.mysql.jdbc.Driver");
            
            Properties props = new Properties();
            props.setProperty("HOST", host);
            props.setProperty("PORT", port);
            props.setProperty("DBNAME", dbname);
            props.setProperty("user", user);
            props.setProperty("password", password);        
            props.setProperty("useServerPrepStmts", "true");
            props.setProperty("blobSendChunkSize", "16384");
            
            if (jar_version.equals("new")) {
                URL = "jdbc:mysql://?allowPublicKeyRetrieval=true&useServerPrepStmts=true&serverTimezone=UTC";
            } else {
                URL = "jdbc:mysql://?allowPublicKeyRetrieval=true&useServerPrepStmts=true&serverTimezone=UTC&useSSL=false";
            }
            
            conn = DriverManager.getConnection(URL, props);
            
            createTable(conn);
            
            insertLongTextData(conn);
            
            insertLongBinaryData(conn);
            
            readData(conn);
            
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            try {
                if (fis != null) fis.close();
                if (pstmt != null) pstmt.close();
                if (conn != null) conn.close();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }
    
    private static void createTable(Connection conn) throws SQLException {
        String createTableSQL = 
            "CREATE TABLE IF NOT EXISTS long_data_test (" +
            "id INT AUTO_INCREMENT PRIMARY KEY, " +
            "file_name VARCHAR(255), " +
            "large_text LONGTEXT, " +
            "binary_data LONGBLOB, " +
            "created_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP, " +
            "alias_name VARCHAR(255) " +
            ")";
        
        try (Statement stmt = conn.createStatement()) {
            stmt.execute("drop table if exists long_data_test");
            stmt.execute(createTableSQL);
            System.out.println("CREATE TABLE SUCCESS");
        }
    }
    
    private static void insertLongTextData(Connection conn) throws SQLException, IOException {
        String insertSQL = "INSERT INTO long_data_test (file_name, large_text, alias_name) VALUES ($1, $2, $3)";
        
        try (PreparedStatement pstmt = conn.prepareStatement(insertSQL)) {
            pstmt.setString(1, "large_text_file.txt");
            
            StringBuilder largeText = new StringBuilder();
            for (int i = 0; i < 10000; i++) {
                largeText.append("this is ").append(i).append("th row data.");
                largeText.append("JDBC send with COM_STMT_SEND_LONG_DATA\n");
            }
            
            StringReader reader = new StringReader(largeText.toString());
            pstmt.setCharacterStream(2, reader, largeText.length());

            pstmt.setString(3, "protocol.txt");
            
            int rows = pstmt.executeUpdate();
            System.out.println("insert success, affected rows: " + rows);
        }
    }
    
    private static void insertLongBinaryData(Connection conn) throws SQLException, IOException {
        String insertSQL = "INSERT INTO long_data_test (file_name, binary_data, alias_name) VALUES ($1, $2, $3)";
        
        try (PreparedStatement pstmt = conn.prepareStatement(insertSQL)) {
            pstmt.setString(1, "large_binary_file.dat");
            
            byte[] largeBinaryData = createLargeBinaryData(2 * 1024 * 1024); // 2MB
            
            ByteArrayInputStream bis = new ByteArrayInputStream(largeBinaryData);
            pstmt.setBinaryStream(2, bis, largeBinaryData.length);
            
            pstmt.setString(3, "protocol.dat");

            int rows = pstmt.executeUpdate();
            System.out.println("insert blob data success, affected rows: " + rows);
        }
    }
    
    private static void insertBinaryDataFromFile(Connection conn, String filePath) 
            throws SQLException, IOException {
        
        String insertSQL = "INSERT INTO long_data_test (file_name, binary_data, alias_name) VALUES ($1, $2, $3)";
        File file = new File(filePath);
        
        try (PreparedStatement pstmt = conn.prepareStatement(insertSQL);
             FileInputStream fis = new FileInputStream(file)) {
            
            pstmt.setString(1, file.getName());
            pstmt.setBinaryStream(2, fis, (int) file.length());

            pstmt.setString(3, "protocol.dat");
            
            int rows = pstmt.executeUpdate();
            System.out.println("insert blob data from file success, affected rows: " + rows);
        }
    }

    private static void readData(Connection conn) throws SQLException {
        String querySQL = "SELECT id, file_name, LENGTH(large_text) as text_length, " +
                         "LENGTH(binary_data) as binary_length, created_time, alias_name " +
                         "FROM long_data_test ORDER BY id";
        
        try (Statement stmt = conn.createStatement();
             ResultSet rs = stmt.executeQuery(querySQL)) {
            
            System.out.println("\nResult Check:");
            System.out.println("ID\tFileName\t\t\tTextLength\tBinLength");
            System.out.println("------------------------------------------------------------");
            
            while (rs.next()) {
                int id = rs.getInt("id");
                String fileName = rs.getString("file_name");
                long textLength = rs.getLong("text_length");
                long binaryLength = rs.getLong("binary_length");
                
                System.out.printf("%d\t%-20s\t%d\t%d\n", 
                    id, fileName, textLength, binaryLength);
            }
        }
    }
    
    private static byte[] createLargeBinaryData(int size) {
        byte[] data = new byte[size];
        for (int i = 0; i < size; i++) {
            data[i] = (byte) (i % 256);
        }
        return data;
    }
    
    private static void insertUsingSetBlob(Connection conn) throws SQLException, IOException {
        String insertSQL = "INSERT INTO long_data_test (file_name, binary_data, alias_name) VALUES ($1, $2, $3)";
        
        try (PreparedStatement pstmt = conn.prepareStatement(insertSQL)) {
            pstmt.setString(1, "blob_example.dat");
            
            byte[] largeData = createLargeBinaryData(1024 * 1024); // 1MB
            
            ByteArrayInputStream bis = new ByteArrayInputStream(largeData);
            pstmt.setBlob(2, bis, largeData.length);
            
            pstmt.setString(3, "protocol.dat");

            int rows = pstmt.executeUpdate();
            System.out.println("Insert with setBlob() success, affected rows: " + rows);
        }
    }
}