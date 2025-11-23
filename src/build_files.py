import sys
import os
import re
import hashlib
from pathlib import Path
from datetime import datetime
import system_init

WORDLIST_LANGUAGES = ['en', 'es', 'fr', 'it', 'pt', 'cs', 'jp', 'ko', 'zh_s', 'zh_t']
WORDLIST_SIZE = 2048
WORD_BITS = 11


def validate_wordlist_file(filepath):
    if not os.path.exists(filepath):
        return False
    with open(filepath, 'r', encoding='utf-8') as f:
        words = [line.strip() for line in f if line.strip()]
    return len(words) == WORDLIST_SIZE

def calculate_wordlist_checksum(filepath):
    hasher = hashlib.sha256()
    with open(filepath, 'rb') as f:
        hasher.update(f.read())
    return hasher.hexdigest()

def normalize_word_input(word):
    word = word.lower().strip()
    word = re.sub(r'[^a-z]', '', word)
    return word

def find_word_matches(prefix, wordlist):
    prefix = normalize_word_input(prefix)
    if len(prefix) < 4:
        return []
    matches = [w for w in wordlist if w.startswith(prefix)]
    return matches

def load_wordlist(lang_code):
    src_dir = Path(__file__).parent
    wordlist_file = src_dir / f'lang_{lang_code}.c'
    
    if not wordlist_file.exists():
        return None
    
    words = []
    with open(wordlist_file, 'r', encoding='utf-8') as f:
        content = f.read()
        words_section_match = re.search(r'\.words\s*=\s*\{([^}]+)\}', content, re.DOTALL)
        if words_section_match:
            words_section = words_section_match.group(1)
            pattern = r'u8"([^"]+)"|"([^"]+)"'
            matches = re.findall(pattern, words_section)
            words = [m[0] or m[1] for m in matches if (m[0] or m[1]) and len(m[0] or m[1]) > 0]
    
    return words if len(words) == WORDLIST_SIZE else None

def verify_wordlist_integrity(lang_code):
    words = load_wordlist(lang_code)
    if not words:
        return False
    
    if len(words) != WORDLIST_SIZE:
        return False
    
    if len(set(words)) != len(words):
        return False
    
    return True

def get_available_languages():
    available = []
    for lang in WORDLIST_LANGUAGES:
        if verify_wordlist_integrity(lang):
            available.append(lang)
    return available

def generate_build_report():
    report = []
    report.append(f"Polyseed Wordlist Verification Report")
    report.append(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    report.append("="*60)
    
    for lang in WORDLIST_LANGUAGES:
        status = "OK" if verify_wordlist_integrity(lang) else "FAILED"
        report.append(f"Language {lang:8s}: {status}")
    
    report.append("="*60)
    return "\n".join(report)

def main():
    print(generate_build_report())
    available = get_available_languages()
    print(f"\nAvailable languages: {len(available)}/{len(WORDLIST_LANGUAGES)}")
    return 0

if __name__ == "__main__":
    sys.exit(main())
