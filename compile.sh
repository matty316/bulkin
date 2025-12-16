OS_NAME=$(uname -s)

if [ "$OS_NAME" = "Darwin" ]; then
    echo "This is an Apple macOS system."
    ./compile-mac.sh
else
    echo "This is not an Apple macOS system (Operating System detected: $OS_NAME)."
    ./compile-nix.sh
fi
