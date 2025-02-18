import matplotlib.pyplot as plt
import pandas as pd

# Load data from the generated line-count.dat file
def load_data(file_path):
    """Load and parse the line-count.dat file."""
    data = pd.read_csv(file_path, sep=" ", header=None, names=["date", "source_lines", "test_lines"])
    data["date"] = pd.to_datetime(data["date"])
    return data

# Plot the data
def plot_data(data):
    """Visualize the source and test lines over time."""
    plt.figure(figsize=(12, 6))

    # Plot source lines
    plt.plot(data["date"], data["source_lines"], label="Lines of code in source files", color="blue")

    # Plot test lines
    plt.plot(data["date"], data["test_lines"], label="Lines of code in test files", color="orange")

    # Formatting the plot
    plt.xlabel("Date")
    plt.ylabel("Lines of Code")
    plt.title("Growth of Lines of Code Over Time")
    plt.legend()
    plt.grid(True, linestyle="--", alpha=0.7)

    # Save and show plot
    plt.tight_layout()
    plt.savefig("line_count_growth.png")
    plt.show()

if __name__ == "__main__":
    # File path to line-count.dat
    file_path = "line-count.dat"

    try:
        # Load the data
        data = load_data(file_path)

        # Visualize the data
        plot_data(data)

    except FileNotFoundError:
        print(f"Error: The file '{file_path}' was not found. Make sure the bash script output is saved to this path.")
    except Exception as e:
        print(f"An error occurred: {e}")
