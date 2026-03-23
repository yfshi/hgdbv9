import java.sql.*;
import java.util.Properties;
import java.io.*;

public class MySQLInsertLongData {
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
    }

    private static String read_file_to_string(String file_path) throws IOException
    {
        StringBuilder sb = new StringBuilder();
        FileReader fr = new FileReader(file_path);
        BufferedReader br = new BufferedReader(fr);

        String line;
        while((line = br.readLine()) != null)
        {
            sb.append(line);
            sb.append('\n');
        }

        br.close();

        return sb.toString();
    }

    private static void initTestData() {
        String createTableSQL = "CREATE TABLE IF NOT EXISTS test_long_sql (content longtext)";
        String insertSQL = "INSERT INTO test_long_sql (content) VALUES (?)";
        String file_path = "data/longdata.data";
        
        try (Connection conn = DriverManager.getConnection(url_jdbc, info);
            Statement stmt = conn.createStatement()) {

            String file_data = read_file_to_string(file_path);

            // Create table
            stmt.executeUpdate(createTableSQL);
            stmt.executeUpdate("truncate test_long_sql");

            stmt.execute("set standard_conforming_strings = off");

            try (PreparedStatement pstmt = conn.prepareStatement(insertSQL)) {
                pstmt.setString(1, file_data);
                pstmt.executeUpdate();
                System.out.println("Insert success, data length is " + file_data.length());

                file_data = file_data.substring(0, 0xffffff - 45 - 356685 - 1950 + 21);
                pstmt.setString(1, file_data);
                pstmt.executeUpdate();
                System.out.println("Insert success, data length is " + file_data.length());
            }
            
            System.out.println("Insert success, data length is " + file_data.length());

            ResultSet rs = stmt.executeQuery("SELECT content FROM test_long_sql");
            while (rs.next()) {
                System.out.println("Content length is " + rs.getString(1).length());
            }

        } catch (SQLException e) {
            e.printStackTrace();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
    }
}