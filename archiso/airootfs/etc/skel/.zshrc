# ~/.zshrc — NicoOS 27 Extreme
# Zsh è la shell predefinita di NicoOS.

# --- cronologia -------------------------------------------------------------
HISTFILE=~/.zsh_history
HISTSIZE=50000
SAVEHIST=50000
setopt HIST_IGNORE_ALL_DUPS HIST_REDUCE_BLANKS SHARE_HISTORY EXTENDED_HISTORY

# --- completamento / editing ------------------------------------------------
autoload -Uz compinit && compinit -u
autoload -Uz up-line-or-beginning-search down-line-or-beginning-search
zle -N up-line-or-beginning-search
zle -N down-line-or-beginning-search
bindkey '^[[A' up-line-or-beginning-search
bindkey '^[[B' down-line-or-beginning-search
setopt AUTO_CD AUTO_PUSHD PUSHD_IGNORE_DUPS INTERACTIVE_COMMENTS

# --- prompt: minimale, scuro, con accento -----------------------------------
autoload -Uz colors && colors
PROMPT='%F{#e3272b}%n%f %F{#5a6b80}%~%f %F{#b41e22}❯%f '
RPROMPT='%F{#5a6b80}%?%f'

# --- alias ------------------------------------------------------------------
alias ls='ls --color=auto'
alias ll='ls -lAh --color=auto'
alias grep='grep --color=auto'
alias pac='sudo pacman'
alias update='sudo pacman -Syu'
alias install='sudo pacman -S'
alias remove='sudo pacman -Rns'
alias search='pacman -Ss'
alias nicoos-shell-log='tail -f ~/.cache/nicoos/shell.log 2>/dev/null || journalctl --user -b'

# --- ambiente ---------------------------------------------------------------
export EDITOR="${EDITOR:-nano}"
export VISUAL="$EDITOR"
export PAGER=less
export MOZ_ENABLE_WAYLAND=1
export QT_QPA_PLATFORM=wayland

# Aggiungi ~/.local/bin al PATH
case ":$PATH:" in
    *":$HOME/.local/bin:"*) ;;
    *) PATH="$HOME/.local/bin:$PATH" ;;
esac
export PATH
