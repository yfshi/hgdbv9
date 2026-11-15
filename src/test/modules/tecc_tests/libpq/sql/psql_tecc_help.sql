-- ReqID:TECC4.0 - psql TECC SQL help smoke test
\h CREATE KEK
\h ALTER KEK
\h CREATE DEK
\h DROP KEK
\h DROP DEK
\! psql --help=commands | sed -n '/^TECC$/,/^Formatting$/p'
