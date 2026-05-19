/* Begin - BUG#7733 */
import java.sql.Connection;
import java.sql.DatabaseMetaData;
import java.sql.DriverManager;
import java.util.Properties;

public class MySQLJdbcGetColumnsTest {
    public static void main(String[] args) throws Exception {
        String host = args[0];
        String port = args[1];
        String dbname = args[2];
        String user = args[3];
        String password = args[4];

        Properties info = new Properties();
        info.setProperty("HOST", host);
        info.setProperty("PORT", port);
        info.setProperty("DBNAME", dbname);
        info.setProperty("user", user);
        info.setProperty("password", password);

        String url = "jdbc:mysql://?allowPublicKeyRetrieval=true&useSSL=false";
        try (Connection conn = DriverManager.getConnection(url, info)) {
            DatabaseMetaData meta = conn.getMetaData();
            meta.getColumns(null, null, null, null);
        }
    }
}
/* END - BUG#7733 */