bas55=$top_builddir/ecma55/bas55
bas=$nom.BAS
out=$nom.out
err=$nom.err
ok=$nom.ok
eok=$nom.eok

$bas55 $bas >$out 2>$err

diff $out $ok && diff $err $eok && rm -f $out $err
