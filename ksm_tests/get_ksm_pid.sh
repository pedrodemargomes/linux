for stat_file in /proc/[0-9]*/ksm_stat
do
	echo $stat_file
	if [ -f "$stat_file" ]
	then
		# Read the number of sharing pages
		sharing=$(cat "$stat_file")
		# Only print processes actively saving memory
		if [ -n "$sharing" ]
		then
			echo $sharing
			cat "$stat_file" | grep "ksm_merging_pages"
		fi
	fi
done
