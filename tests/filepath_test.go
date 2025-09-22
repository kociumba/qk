package main

import (
	"path/filepath"
	"runtime"
	"strings"
	"testing"
)

func TestFilepathClean(t *testing.T) {
	t.Run("Basic path cleaning", func(t *testing.T) {
		tests := []struct {
			input    string
			expected string
		}{
			{"path/to/../file", "path/file"},
			{"path/./file", "path/file"},
			{filepath.Join("path", "", "file"), "path/file"},
		}
		for _, tt := range tests {
			t.Run(tt.input, func(t *testing.T) {
				got := filepath.Clean(tt.input)
				expected := filepath.FromSlash(tt.expected)
				if got != expected {
					t.Errorf("Clean(%q) = %q; want %q", tt.input, got, expected)
				}
			})
		}
	})

	t.Run("Rooted paths", func(t *testing.T) {
		tests := []struct {
			input    string
			expected string
		}{
			{"/path/to/../file", "/path/file"},
			{"/.", "/"},
			{"/..", "/"},
		}
		for _, tt := range tests {
			t.Run(tt.input, func(t *testing.T) {
				got := filepath.Clean(tt.input)
				expected := filepath.FromSlash(tt.expected)
				if got != expected {
					t.Errorf("Clean(%q) = %q; want %q", tt.input, got, expected)
				}
			})
		}
	})

	t.Run("Additional edge cases", func(t *testing.T) {
		tests := []struct {
			input    string
			expected string
		}{
			{filepath.FromSlash("path\\to/file"), "path/to/file"},
			{"a/b/../../c", "c"},
			{"C:foo/..", "C:."},
			{"\\\\?\\C:\\foo\\..", "\\\\?\\C:\\"},
		}
		for _, tt := range tests {
			t.Run(tt.input, func(t *testing.T) {
				got := filepath.Clean(tt.input)
				expected := tt.expected
				if runtime.GOOS == "windows" {
					expected = strings.ReplaceAll(tt.expected, "/", "\\")
				} else {
					expected = filepath.FromSlash(tt.expected)
				}
				if got != expected {
					t.Errorf("Clean(%q) = %q; want %q", tt.input, got, expected)
				}
			})
		}
	})

	t.Run("Empty or dot paths", func(t *testing.T) {
		tests := []struct {
			input    string
			expected string
		}{
			{"", "."},
			{".", "."},
			{"..", ".."},
		}
		for _, tt := range tests {
			t.Run(tt.input, func(t *testing.T) {
				got := filepath.Clean(tt.input)
				if got != tt.expected {
					t.Errorf("Clean(%q) = %q; want %q", tt.input, got, tt.expected)
				}
			})
		}
	})

	if runtime.GOOS == "windows" {
		t.Run("Windows volume name handling", func(t *testing.T) {
			tests := []struct {
				input    string
				expected string
			}{
				{"C:\\path\\to\\..\\file", "C:\\path\\file"},
				{"C:\\", "C:\\"},
				{"\\\\server\\share\\path\\..\\file", "\\\\server\\share\\file"},
			}
			for _, tt := range tests {
				t.Run(tt.input, func(t *testing.T) {
					got := filepath.Clean(tt.input)
					if got != tt.expected {
						t.Errorf("Clean(%q) = %q; want %q", tt.input, got, tt.expected)
					}
				})
			}
		})
	}
}

func TestFilepathSplit(t *testing.T) {
	t.Run("Basic path splitting", func(t *testing.T) {
		dir, file := filepath.Split(filepath.FromSlash("path/to/file.txt"))
		wantDir := filepath.FromSlash("path/to/")
		wantFile := "file.txt"
		if dir != wantDir || file != wantFile {
			t.Errorf("Split(%q) = (%q, %q); want (%q, %q)", "path/to/file.txt", dir, file, wantDir, wantFile)
		}
	})

	t.Run("Rooted path splitting", func(t *testing.T) {
		dir, file := filepath.Split(filepath.FromSlash("/file.txt"))
		wantDir := filepath.FromSlash("/")
		wantFile := "file.txt"
		if dir != wantDir || file != wantFile {
			t.Errorf("Split(%q) = (%q, %q); want (%q, %q)", "/file.txt", dir, file, wantDir, wantFile)
		}
	})

	t.Run("No directory", func(t *testing.T) {
		dir, file := filepath.Split("file.txt")
		wantDir := ""
		wantFile := "file.txt"
		if dir != wantDir || file != wantFile {
			t.Errorf("Split(%q) = (%q, %q); want (%q, %q)", "file.txt", dir, file, wantDir, wantFile)
		}
	})

	t.Run("Edge cases", func(t *testing.T) {
		tests := []struct {
			input    string
			wantDir  string
			wantFile string
		}{
			{"", "", ""},
			{filepath.FromSlash("path/"), filepath.FromSlash("path/"), ""},
			{"C:", "C:", ""},
			{"\\\\server\\share", "\\\\server\\share", ""},
		}
		for _, tt := range tests {
			t.Run(tt.input, func(t *testing.T) {
				dir, file := filepath.Split(tt.input)
				wantDir := tt.wantDir
				if runtime.GOOS == "windows" {
					wantDir = strings.ReplaceAll(tt.wantDir, "/", "\\")
				}
				if dir != wantDir || file != tt.wantFile {
					t.Errorf("Split(%q) = (%q, %q); want (%q, %q)", tt.input, dir, file, wantDir, tt.wantFile)
				}
			})
		}
	})

	if runtime.GOOS == "windows" {
		t.Run("Windows volume name", func(t *testing.T) {
			dir, file := filepath.Split("C:\\path\\file.txt")
			wantDir := "C:\\path\\"
			wantFile := "file.txt"
			if dir != wantDir || file != wantFile {
				t.Errorf("Split(%q) = (%q, %q); want (%q, %q)", "C:\\path\\file.txt", dir, file, wantDir, wantFile)
			}
		})
	}
}

func TestFilepathExt(t *testing.T) {
	t.Run("Basic extension extraction", func(t *testing.T) {
		tests := []struct {
			input    string
			expected string
		}{
			{"file.txt", ".txt"},
			{"file.tar.gz", ".gz"},
			{"file", ""},
		}
		for _, tt := range tests {
			t.Run(tt.input, func(t *testing.T) {
				got := filepath.Ext(tt.input)
				if got != tt.expected {
					t.Errorf("Ext(%q) = %q; want %q", tt.input, got, tt.expected)
				}
			})
		}
	})

	t.Run("Additional cases", func(t *testing.T) {
		tests := []struct {
			input    string
			expected string
		}{
			{".gitignore", ".gitignore"},
			{"file..txt", ".txt"},
			{filepath.FromSlash("path/."), "."},
		}
		for _, tt := range tests {
			t.Run(tt.input, func(t *testing.T) {
				got := filepath.Ext(tt.input)
				if got != tt.expected {
					t.Errorf("Ext(%q) = %q; want %q", tt.input, got, tt.expected)
				}
			})
		}
	})

	t.Run("Path with directories", func(t *testing.T) {
		got := filepath.Ext(filepath.FromSlash("path/to/file.txt"))
		want := ".txt"
		if got != want {
			t.Errorf("Ext(%q) = %q; want %q", "path/to/file.txt", got, want)
		}
	})

	t.Run("No extension", func(t *testing.T) {
		got := filepath.Ext(filepath.FromSlash("path/to/file"))
		want := ""
		if got != want {
			t.Errorf("Ext(%q) = %q; want %q", "path/to/file", got, want)
		}
	})
}

func TestFilepathBase(t *testing.T) {
	t.Run("Basic base extraction", func(t *testing.T) {
		tests := []struct {
			input    string
			expected string
		}{
			{filepath.FromSlash("path/to/file.txt"), "file.txt"},
			{"file.txt", "file.txt"},
			{"", "."},
		}
		for _, tt := range tests {
			t.Run(tt.input, func(t *testing.T) {
				got := filepath.Base(tt.input)
				if got != tt.expected {
					t.Errorf("Base(%q) = %q; want %q", tt.input, got, tt.expected)
				}
			})
		}
	})

	t.Run("Rooted paths", func(t *testing.T) {
		tests := []struct {
			input    string
			expected string
		}{
			{filepath.FromSlash("/path/to/file.txt"), "file.txt"},
		}
		if runtime.GOOS == "windows" {
			tests = append(tests, struct {
				input    string
				expected string
			}{"C:\\path\\to\\file.txt", "file.txt"})
		}
		for _, tt := range tests {
			t.Run(tt.input, func(t *testing.T) {
				got := filepath.Base(tt.input)
				if got != tt.expected {
					t.Errorf("Base(%q) = %q; want %q", tt.input, got, tt.expected)
				}
			})
		}
	})

	t.Run("Trailing separators", func(t *testing.T) {
		got := filepath.Base(filepath.FromSlash("path/to//"))
		expected := "to"
		if got != expected {
			t.Errorf("Base(%q) = %q; want %q", "path/to//", got, expected)
		}
	})

	if runtime.GOOS == "windows" {
		t.Run("Windows edge cases", func(t *testing.T) {
			got := filepath.Base("C:")
			expected := string(filepath.Separator)
			if got != expected {
				t.Errorf("Base(%q) = %q; want %q", "C:", got, expected)
			}
		})
	}
}

func TestFilepathDir(t *testing.T) {
	t.Run("Basic directory extraction", func(t *testing.T) {
		tests := []struct {
			input    string
			expected string
		}{
			{filepath.FromSlash("path/to/file.txt"), "path/to"},
			{"file.txt", "."},
		}
		for _, tt := range tests {
			t.Run(tt.input, func(t *testing.T) {
				got := filepath.Dir(tt.input)
				expected := filepath.FromSlash(tt.expected)
				if got != expected {
					t.Errorf("Dir(%q) = %q; want %q", tt.input, got, expected)
				}
			})
		}
	})

	t.Run("Rooted paths", func(t *testing.T) {
		tests := []struct {
			input    string
			expected string
		}{
			{filepath.FromSlash("/path/to/file.txt"), "/path/to"},
		}
		if runtime.GOOS == "windows" {
			tests = append(tests, struct {
				input    string
				expected string
			}{"C:\\path\\to\\file.txt", "C:\\path\\to"})
		}
		for _, tt := range tests {
			t.Run(tt.input, func(t *testing.T) {
				got := filepath.Dir(tt.input)
				expected := tt.expected
				if runtime.GOOS == "windows" {
					expected = strings.ReplaceAll(expected, "/", "\\")
				} else {
					expected = filepath.FromSlash(expected)
				}
				if got != expected {
					t.Errorf("Dir(%q) = %q; want %q", tt.input, got, expected)
				}
			})
		}
	})

	t.Run("Edge cases", func(t *testing.T) {
		tests := []struct {
			input    string
			expected string
		}{
			{"", "."},
			{"C:", "C:."},
			{"\\\\server\\share\\file", "\\\\server\\share\\"},
			{filepath.FromSlash("path/"), "path"},
		}
		for _, tt := range tests {
			t.Run(tt.input, func(t *testing.T) {
				got := filepath.Dir(tt.input)
				expected := tt.expected
				if runtime.GOOS == "windows" {
					expected = strings.ReplaceAll(expected, "/", "\\")
				} else {
					expected = filepath.FromSlash(expected)
				}
				if got != expected {
					t.Errorf("Dir(%q) = %q; want %q", tt.input, got, expected)
				}
			})
		}
	})
}

func TestFilepathSlashConversion(t *testing.T) {
	t.Run("To slash", func(t *testing.T) {
		tests := []struct {
			input    string
			expected string
		}{
			{filepath.Join("path", "to", "file"), "path/to/file"},
			{"path/to/file", "path/to/file"},
			{"", ""},
			{string(filepath.Separator), "/"},
		}
		for _, tt := range tests {
			t.Run(tt.input, func(t *testing.T) {
				got := filepath.ToSlash(tt.input)
				if got != tt.expected {
					t.Errorf("ToSlash(%q) = %q; want %q", tt.input, got, tt.expected)
				}
			})
		}
	})

	t.Run("From slash", func(t *testing.T) {
		tests := []struct {
			input    string
			expected string
		}{
			{"path/to/file", filepath.Join("path", "to", "file")},
			{"", ""},
		}
		for _, tt := range tests {
			t.Run(tt.input, func(t *testing.T) {
				got := filepath.FromSlash(tt.input)
				if got != tt.expected {
					t.Errorf("FromSlash(%q) = %q; want %q", tt.input, got, tt.expected)
				}
			})
		}
	})

	t.Run("Edge cases", func(t *testing.T) {
		// Already covered in above
	})
}

func TestFilepathVolumeName(t *testing.T) {
	t.Run("Non-Windows volume name", func(t *testing.T) {
		got := filepath.VolumeName(filepath.FromSlash("path/to/file"))
		want := ""
		if got != want {
			t.Errorf("VolumeName(%q) = %q; want %q", "path/to/file", got, want)
		}
	})

	if runtime.GOOS == "windows" {
		t.Run("Windows volume name", func(t *testing.T) {
			tests := []struct {
				input    string
				expected string
			}{
				{"C:\\path\\to\\file", "C:"},
				{"\\\\server\\share\\file", "\\\\server\\share"},
			}
			for _, tt := range tests {
				t.Run(tt.input, func(t *testing.T) {
					got := filepath.VolumeName(tt.input)
					if got != tt.expected {
						t.Errorf("VolumeName(%q) = %q; want %q", tt.input, got, tt.expected)
					}
				})
			}
		})

		t.Run("Windows edge cases", func(t *testing.T) {
			tests := []struct {
				input    string
				expected string
			}{
				{"\\\\?\\C:\\file", "\\\\?\\C:"},
				{"\\\\server", "\\\\server"},
				{"C", ""},
			}
			for _, tt := range tests {
				t.Run(tt.input, func(t *testing.T) {
					got := filepath.VolumeName(tt.input)
					if got != tt.expected {
						t.Errorf("VolumeName(%q) = %q; want %q", tt.input, got, tt.expected)
					}
				})
			}
		})
	}
}
