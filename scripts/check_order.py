import sys
from colorama import Fore, Style, init

def check_ascending_numbers(file_path):
    init()  # Initialize colorama
    with open(file_path, 'r') as file:
        previous_number = None
        all_ascending = True
        for lineno, line in enumerate(file, start=1):
            current_number = int(line.split('.')[0])
            if previous_number is not None and current_number <= previous_number:
                print(f"{Fore.RED}ERROR{Style.RESET_ALL}: Line {lineno} not in ascending order")
                all_ascending = False
            previous_number = current_number
        
        if all_ascending:
            print(f"{Fore.GREEN}PASSED{Style.RESET_ALL}: All numbers are in ascending order.")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python script.py <file_path>")
        sys.exit(1)

    file_path = sys.argv[1]
    check_ascending_numbers(file_path)
