-- ReqID:TECC4.0 - psql TECC session mode meta-command smoke test
\show hg_tecc.mode
\disable hg_tecc.mode
\show hg_tecc.mode
\enable hg_tecc.mode
\show hg_tecc.mode
\! psql -k off -X -A -t -d postgres -c "\\show hg_tecc.mode" -c "\\disable hg_tecc.mode" -c "\\show hg_tecc.mode" -c "\\enable hg_tecc.mode" -c "\\show hg_tecc.mode"
