# Add/remove the paths for the rebuilt OpenSSL.
# Option -u unsets the environment variables.
[[ $1 == -u ]] && setenv_unset="-r" || setenv_unset=

# Add an element to a search path.
# Syntax: pathmunge [-p varname] [-a] [-t] dirname
# With -a: append last, -t: add trailing colon, -r: remove element instead of adding it.
pathmunge () {
    local dirname=
    local varname=PATH
    local after=false
    local remove=false
    local trailing=false
    while [[ $# -gt 0 ]]; do
        case "$1" in
            -p) shift; varname="$1" ;;
            -a) after=true ;;
            -r) remove=true ;;
            -t) trailing=true ;;
            -*) echo >&2 "pathmunge: invalid option $1" ;;
            *)  dirname="$1" ;;
        esac
        shift
    done
    if [[ -z "$varname" ]]; then
        echo >&2 "pathmunge: empty environment variable name, $dirname not added"
    elif [[ -z "$dirname" ]]; then
        echo >&2 "pathmunge: empty directory not added to $varname"
    else
        # Remove all previous occurrences
        local path=":${!varname}:"
        path="${path//:$dirname:/:}"
        # Add new element.
        if ! $remove; then
            $after && path="$path$dirname" || path="$dirname$path"
        fi
        # Cleanup path
        while [[ "$path" == :* ]]; do path="${path/#:/}"; done
        while [[ "$path" == *: ]]; do path="${path/%:/}"; done
        path="${path//::/:}"
        $trailing && path="$path:"
        export $varname="$path"
    fi
}

export OSSLROOT=$(cd $(dirname "$BASH_SOURCE[0]"); pwd)/.openssl/usr/local
pathmunge $setenv_unset "$OSSLROOT/bin"
if [[ -d "$OSSLROOT/lib64" ]]; then
    pathmunge $setenv_unset -p LD_LIBRARY_PATH "$OSSLROOT/lib64"
else
    pathmunge $setenv_unset -p LD_LIBRARY_PATH "$OSSLROOT/lib"
fi
