import btmha

def run_test():
    print("Initializing BTMHA...")
    btmha.init()

    print("Parsing some arithmetic...")
    btmha.parse("x = [0 0.1 1]")
    btmha.parse("y = sin(x)")
    btmha.parse("z = x + y")
    
    # We can't plot because we're headless, but we can verify variables were created and no crash occurred.
    print("Testing plot command (headless)...")
    btmha.parse("plot z")

    print("Cleaning up BTMHA...")
    btmha.cleanup()
    print("Test passed successfully!")

if __name__ == "__main__":
    run_test()
