import java.io.File;
import java.io.FileWriter;
import java.sql.*;
import java.util.Properties;
import java.math.BigDecimal;

public class MySQLJdbcLoadDataLocalOffTest {
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
			url_jdbc = "jdbc:mysql://?allowLoadLocalInfile=false&?allowPublicKeyRetrieval=true&useServerPrepStmts=true&serverTimezone=UTC&createDatabaseIfNotExist=true";
		}
		else
		{
			url_jdbc = "jdbc:mysql://?allowLoadLocalInfile=false&?allowPublicKeyRetrieval=true&useServerPrepStmts=true&serverTimezone=UTC&useSSL=false&createDatabaseIfNotExist=true";
		}

		try (Connection connection = DriverManager.getConnection(url_jdbc, info);
				Statement statement = connection.createStatement())
		{
			statement.executeUpdate("drop table if exists t_load");
			statement.executeUpdate("create table t_load (" +
								"id int," +
								"name varchar(10)," +
								"PRIMARY KEY (id)" +
								")");

			String workerDir = System.getProperty("user.dir");
			File csv = new File(workerDir, "t_load.csv");

			String sql = "LOAD DATA LOCAL INFILE '" + csv.getAbsolutePath() + "' "
								+ "INTO TABLE t_load "
								+ "FIELDS TERMINATED BY ','";

			statement.execute(sql);
		}
	}
}
