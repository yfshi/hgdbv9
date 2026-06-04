import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.Statement;
import java.util.Properties;

public class MySQLJdbcFetchTest2 {
    private static String host;
    private static String port = "3307";
    private static String dbname;
    private static String user;
    private static String password;
    private static String jar_version = "old";
    private static String url_jdbc;

    public static void main(String[] args) throws Exception {
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

        Properties info = new Properties();
        info.setProperty("HOST", host);
        info.setProperty("PORT", port);
        info.setProperty("DBNAME", dbname);
        info.setProperty("user", user);
        info.setProperty("password", password);
        
        if (jar_version.equals("new")) {
            url_jdbc = "jdbc:mysql://?allowPublicKeyRetrieval=true&useServerPrepStmts=true&serverTimezone=UTC";
        } else {
            url_jdbc = "jdbc:mysql://?allowPublicKeyRetrieval=true&useServerPrepStmts=true&serverTimezone=UTC&useSSL=false";
        }

        try (Connection connection = DriverManager.getConnection(url_jdbc, info);
             Statement statement = connection.createStatement()) {
            
            statement.execute("CREATE OR REPLACE PROCEDURE jdbc_fetch_test(INOUT result_cursor REFCURSOR) LANGUAGE plpgsql AS $$ DECLARE temp_count INTEGER; BEGIN OPEN result_cursor FOR select relname from pg_class where relname = 'pg_class'; END; $$;");

            statement.execute("begin");
            statement.execute("CALL jdbc_fetch_test('result_cursor')");
            statement.execute("fetch all in `result_cursor`");
            statement.execute("end");

            statement.close();
            connection.close();
        }
    }
}
