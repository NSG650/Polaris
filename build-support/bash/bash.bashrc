#
# /etc/bash.bashrc
#

# If not running interactively, don't do anything
[[ $- != *i* ]] && return

[[ $DISPLAY ]] && shopt -s checkwinsize

HISTCONTROL=ignoredups
HISTSIZE=-1
HISTFILESIZE=-1

if [[ $EUID == 0 ]] ; then
  PS1="\[\e[m\][\t] \[\e[1;31m\]\u\[\e[m\]\[\e[1;31m\]@\[\e[m\]\[\e[1;31m\]\h\[\e[m\] \[\e[1;34m\]\w\[\e[m\]# "
else
  PS1="\[\e[m\][\t] \[\e[1;32m\]\u\[\e[m\]\[\e[1;32m\]@\[\e[m\]\[\e[1;32m\]\h\[\e[m\] \[\e[1;34m\]\w\[\e[m\]$ "
fi

unset RED GREEN NORMAL

alias ls='ls --color=auto'
alias grep='grep --color=auto'
alias diff='diff --color=auto'

case ${TERM} in
  Eterm*|alacritty*|aterm*|foot*|gnome*|konsole*|kterm*|putty*|rxvt*|tmux*|xterm*)
    PROMPT_COMMAND+=('printf "\033]0;%s@%s:%s\007" "${USER}" "${HOSTNAME%%.*}" "${PWD/#$HOME/\~}"')

    ;;
  screen*)
    PROMPT_COMMAND+=('printf "\033_%s@%s:%s\033\\" "${USER}" "${HOSTNAME%%.*}" "${PWD/#$HOME/\~}"')
    ;;
esac

if [[ -r /usr/share/bash-completion/bash_completion ]]; then
  . /usr/share/bash-completion/bash_completion
fi

cat /etc/motd
