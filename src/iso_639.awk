BEGIN {
	FS = "|";
}

{
	if ($3 != "") {
		if (length($1) > 3) {
			$1 = substr($1, length($1)-2, 3)
		}
		printf "\t{ .key = \"%s\", .val = \"%s\"},\n",$1,$3
	}
}

END {
	printf "\t{ .key = NULL, .val = NULL },\n"
}
