bas="$srcdir"/$nom.BAS
out="$builddir"/$nom.out
err="$builddir"/$nom.err
etp="$builddir"/$nom.etp
ok="$srcdir"/$nom.ok
eok="$srcdir"/$nom.eok

# Always remove \r for Windows.

$bas55 $bas 2>$etp | tr -d '\r' >$out

# Remove \r from $etp and remove leading path in error strings.

sed 's|\r||
     s|'$srcdir'/||' $etp >$err
rm -f $etp

diff $out $ok && diff $err $eok && rm -f $out $err
