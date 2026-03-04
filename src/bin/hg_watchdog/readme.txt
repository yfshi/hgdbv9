hg_watchdog
	hg_watchdog is a background monitoring daemon designed to supervise PostgreSQL instances.

Startup Methods
	hg_watchdog can be started in the following ways:


Method 1:Direct Execution (Default Configuration)
Start the program directly:
./hg_watchdog

In this mode, the program uses the default configuration file:/etc/hg_watchdog/hg_watchdog.conf.

Configuration File Format
	The configuration file must follow this structure:

instance1-installpath:/home/opt/postgresinsll
instance1-installdata:/home/opt/postgresinsll/data

instance2-installpath:/home/opt/postgresinsll
instance2-installdata:/home/opt/postgresinsll/data1

Each block represents one PostgreSQL instance:
instanceno-installpath – PostgreSQL installation directory
instanceno-installdata – PostgreSQL data directory

Multiple instances can be configured by repeating the block.


Method 2:Specify Configuration File Manually
	You can explicitly specify a configuration file:

./hg_watchdog -f config_file (config_file is absolute path to config file)

Requirements
	The file path must be an absolute path，the filename can be arbitrary.
	The file format must be identical to: /etc/hg_watchdog/hg_watchdog.conf.


Method 3:Run as a System Service (Recommended for Production)
	For production environments, it is recommended to run hg_watchdog as a background service.

Steps：
1、Create a systemd service file.
	sudo vim /etc/systemd/system/hg_watchdog

2、Reload systemd configuration.
	sudo systemctl daemon-reload

3、Start and stop the service.
	sudo systemctl start hg_watchdog
	sudo systemctl stop hg_watchdog
	sudo systemctl enable hg_watchdog

Example:
1、create file hg_watchdog.service:

[Unit]
Description=PostgreSQL Watchdog Daemon
After=network.target
StartLimitIntervalSec=0

[Service]
Type=simple

ExecStart==/usr/local/bin/hg_watchdog -f /etc/hg_watchdog/hg_watchdog.conf
User=markbo
Restart=always
RestartSec=5
KillMode=process

[Install]
WantedBy=multi-user.target



2、Reload systemd configuration.
	sudo systemctl daemon-reload

3、Start the service.
	sudo systemctl start hg_watchdog