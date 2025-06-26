_kerpad_long_opt_completions() {
	local cur prev opts
	COMPREPLY=()
	cur="${COMP_WORDS[COMP_CWORD]}"
	prev="${COMP_WORDS[COMP_CWORD-1]}"
	long_opts="--thickness= --minx=  --maxx= --miny= --maxy= --sleep-time= --name= --always --no-edge-protection --edge-scrolling --scroll-div --disable-double-tap --list= --verbose --help"

	if [[ ${prev} == "--list*" ]]
	then
		COMPREPLY=($(compgen -W "candidates all" -- $cur))
		return 0
	fi
	COMPREPLY=($(compgen -W "$long_opts" -- $cur))
	[[ ${COMPREPLY-} == *= ]] && compopt -o nospace
	return 0
}

_kerpad_compose_short_opt() {
	short_opts="t x X y Y s n a l v h"
	for opt in $short_opts
		do
		echo "${1}${opt}"
	done
}

_kerpad_short_opt_completions() {
	local cur prev opts
	COMPREPLY=()
	cur="${COMP_WORDS[COMP_CWORD]}"
	prev="${COMP_WORDS[COMP_CWORD-1]}"
	opts="$(_kerpad_compose_short_opt $cur)"
	
	COMPREPLY=($(compgen -W "$opts" -- $cur))
	return 0
}

_kerpad_completions() {
	# local cur prev words cword was_split comp_args
	# _comp_initialize -s -- "$@" || return
	
	# if [[ ${prev} == --list ]]
	# then
	# 	_comp_compgen -- -W 'candidates all'
	# 	return 0
	# fi
	local cur prev opts
	COMPREPLY=()
	cur="${COMP_WORDS[COMP_CWORD]}"
	prev="${COMP_WORDS[COMP_CWORD-1]}"
	
	if [[ $cur == --* ]]
	then
		_kerpad_long_opt_completions
		return 0
	fi
	if [[ $cur == -* ]]
	then
		_kerpad_short_opt_completions
		return 0
	fi
}

complete -F _kerpad_completions kerpad
