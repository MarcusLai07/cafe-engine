# GitHub Workflow Guide

Complete guide for managing the Cafe Engine repository on GitHub.

## Repository Info

- **URL:** https://github.com/MarcusLai07/cafe-engine
- **Account:** MarcusLai07 (personal)
- **Email:** Marcus_Lai@live.com

---

## Initial Setup (One-Time)

### Option A: GitHub CLI (Recommended)

```bash
# Install GitHub CLI
brew install gh

# Login to your personal account
gh auth login
# Select: GitHub.com → HTTPS → Login with web browser

# Verify
gh auth status
```

After this, all git push/pull will work automatically.

### Option B: Personal Access Token

1. Go to https://github.com/settings/tokens
2. "Generate new token (classic)"
3. Name: `cafe-engine-mac`
4. Expiration: 90 days or custom
5. Scopes: Check `repo` (full control)
6. Generate and **save the token securely**

```bash
# Store credentials (will prompt once, then remember)
git config --global credential.helper osxkeychain

# On first push, enter:
# Username: MarcusLai07
# Password: <your-token>
```

### Option C: SSH Key

```bash
# Generate SSH key
ssh-keygen -t ed25519 -C "Marcus_Lai@live.com"

# Start ssh-agent
eval "$(ssh-agent -s)"

# Add key
ssh-add ~/.ssh/id_ed25519

# Copy public key
pbcopy < ~/.ssh/id_ed25519.pub

# Add to GitHub:
# 1. Go to https://github.com/settings/keys
# 2. "New SSH key"
# 3. Paste and save

# Change remote to SSH
git remote set-url origin git@github.com:MarcusLai07/cafe-engine.git
```

---

## Git Commands - Complete Reference

### Configuration

```bash
# Set user for THIS repo only
git config user.name "MarcusLai07"
git config user.email "Marcus_Lai@live.com"

# Set user globally (all repos)
git config --global user.name "MarcusLai07"
git config --global user.email "Marcus_Lai@live.com"

# View all config
git config --list

# View specific config
git config user.name
git config user.email

# Set default branch name
git config --global init.defaultBranch main

# Set default editor
git config --global core.editor "code --wait"  # VS Code
git config --global core.editor "nano"          # Nano
git config --global core.editor "vim"           # Vim

# Enable colored output
git config --global color.ui auto

# Set line ending handling (macOS)
git config --global core.autocrlf input

# Create aliases
git config --global alias.st status
git config --global alias.co checkout
git config --global alias.br branch
git config --global alias.ci commit
git config --global alias.lg "log --oneline --graph --all"
```

---

### Getting & Creating Repositories

```bash
# Initialize new repo in current directory
git init

# Initialize with specific branch name
git init -b main

# Clone existing repository
git clone https://github.com/MarcusLai07/cafe-engine.git

# Clone into specific folder
git clone https://github.com/MarcusLai07/cafe-engine.git my-folder

# Clone specific branch
git clone -b phase-3 https://github.com/MarcusLai07/cafe-engine.git

# Shallow clone (faster, less history)
git clone --depth 1 https://github.com/MarcusLai07/cafe-engine.git

# Clone with submodules
git clone --recursive https://github.com/user/repo.git
```

---

### Status & Information

```bash
# Check status (what's changed)
git status

# Short status
git status -s

# Show which branch you're on
git branch --show-current

# Show remote URLs
git remote -v

# Show detailed remote info
git remote show origin

# Show last commit
git log -1

# Show all branches (local)
git branch

# Show all branches (including remote)
git branch -a

# Show branches with last commit
git branch -v

# Show tracking info
git branch -vv
```

---

### Staging & Committing

```bash
# Stage specific file
git add src/main.cpp

# Stage specific folder
git add src/renderer/

# Stage multiple files
git add file1.cpp file2.cpp file3.cpp

# Stage all changes (new, modified, deleted)
git add .
git add -A
git add --all

# Stage only modified and deleted (not new files)
git add -u

# Stage interactively (choose hunks)
git add -p

# Stage with dry-run (see what would be added)
git add -n .

# Unstage a file (keep changes)
git reset HEAD src/main.cpp
git restore --staged src/main.cpp  # Modern alternative

# Commit with message
git commit -m "feat: Add customer spawning"

# Commit with multi-line message
git commit -m "feat: Add customer spawning

- Implemented spawn timer
- Added customer pool
- Connected to entity manager"

# Commit with editor (opens default editor)
git commit

# Commit all tracked changes (skip staging)
git commit -a -m "Quick fix"
git commit -am "Quick fix"  # Shorthand

# Amend last commit (change message or add files)
git commit --amend -m "New message"
git commit --amend --no-edit  # Keep same message, add staged files

# Commit with date
git commit --date="2024-01-15 10:00:00" -m "Message"

# Empty commit (useful for triggering CI)
git commit --allow-empty -m "Trigger build"
```

---

### Pulling & Fetching (Getting Changes from Remote)

```bash
# Fetch changes (download but don't merge)
git fetch

# Fetch from specific remote
git fetch origin

# Fetch specific branch
git fetch origin main

# Fetch all remotes
git fetch --all

# Fetch and prune deleted remote branches
git fetch --prune
git fetch -p  # Shorthand

# Pull (fetch + merge)
git pull

# Pull from specific remote/branch
git pull origin main

# Pull with rebase instead of merge (cleaner history)
git pull --rebase
git pull -r  # Shorthand

# Pull and autostash local changes
git pull --autostash

# Pull specific branch into current
git pull origin feature-branch

# Pull and fast-forward only (fail if can't fast-forward)
git pull --ff-only

# Set pull to rebase by default
git config --global pull.rebase true

# Set pull to fast-forward only by default
git config --global pull.ff only
```

---

### Pushing (Sending Changes to Remote)

```bash
# Push current branch
git push

# Push to specific remote/branch
git push origin main

# Push and set upstream (first push of new branch)
git push -u origin feature-branch
git push --set-upstream origin feature-branch

# Push all branches
git push --all

# Push tags
git push --tags

# Push specific tag
git push origin v1.0.0

# Force push (DANGEROUS - overwrites remote)
git push --force
git push -f

# Force push with lease (safer - fails if remote changed)
git push --force-with-lease

# Delete remote branch
git push origin --delete branch-name
git push origin :branch-name  # Alternative syntax

# Push to different remote branch
git push origin local-branch:remote-branch

# Dry-run (see what would be pushed)
git push --dry-run
git push -n
```

---

### Branching

```bash
# List local branches
git branch

# List remote branches
git branch -r

# List all branches
git branch -a

# List branches with details
git branch -v
git branch -vv  # With tracking info

# Create new branch
git branch feature-name

# Create and switch to new branch
git checkout -b feature-name
git switch -c feature-name  # Modern alternative

# Switch to existing branch
git checkout main
git switch main  # Modern alternative

# Switch to previous branch
git checkout -
git switch -

# Rename current branch
git branch -m new-name

# Rename specific branch
git branch -m old-name new-name

# Delete branch (only if merged)
git branch -d branch-name

# Force delete branch (even if not merged)
git branch -D branch-name

# Delete remote branch
git push origin --delete branch-name

# Create branch from specific commit
git branch feature-name abc1234

# Create branch from tag
git branch hotfix v1.0.0

# Track remote branch
git branch --track local-branch origin/remote-branch

# Set upstream for existing branch
git branch -u origin/main
git branch --set-upstream-to=origin/main
```

---

### Merging

```bash
# Merge branch into current
git merge feature-branch

# Merge with commit message
git merge feature-branch -m "Merge feature into main"

# Merge without fast-forward (always create merge commit)
git merge --no-ff feature-branch

# Fast-forward only (fail if can't)
git merge --ff-only feature-branch

# Abort merge (during conflicts)
git merge --abort

# Continue merge after resolving conflicts
git add .
git merge --continue
# Or just commit
git commit

# Squash merge (combine all commits into one)
git merge --squash feature-branch
git commit -m "Squashed feature"

# Check if branch is merged
git branch --merged
git branch --no-merged
```

---

### Rebasing

```bash
# Rebase current branch onto main
git rebase main

# Rebase onto specific branch
git rebase target-branch

# Interactive rebase (edit, squash, reorder commits)
git rebase -i HEAD~5          # Last 5 commits
git rebase -i main            # All commits since main
git rebase -i --root          # All commits

# Abort rebase
git rebase --abort

# Continue rebase after resolving conflicts
git add .
git rebase --continue

# Skip current commit during rebase
git rebase --skip

# Rebase with autostash
git rebase --autostash main

# Pull with rebase
git pull --rebase origin main
```

**Interactive Rebase Commands:**
```
pick   = use commit
reword = use commit but edit message
edit   = use commit but stop to amend
squash = meld into previous commit
fixup  = like squash but discard message
drop   = remove commit
```

---

### Viewing History & Differences

```bash
# View commit history
git log

# Compact one-line format
git log --oneline

# With graph
git log --graph
git log --oneline --graph

# Show all branches in graph
git log --oneline --graph --all

# Limit number of commits
git log -n 10
git log -10  # Shorthand

# Show commits by author
git log --author="MarcusLai07"

# Show commits in date range
git log --after="2024-01-01" --before="2024-12-31"
git log --since="2 weeks ago"
git log --until="yesterday"

# Search commit messages
git log --grep="fix"
git log --grep="Phase 3"

# Show commits that changed a file
git log -- src/main.cpp
git log -p -- src/main.cpp  # With diff

# Show commits that added/removed string
git log -S "function_name"

# Format output
git log --pretty=format:"%h %an %ar - %s"
git log --pretty=oneline
git log --pretty=short
git log --pretty=full

# Show statistics
git log --stat
git log --shortstat

# Show diff
git diff

# Diff staged changes
git diff --staged
git diff --cached  # Same thing

# Diff between commits
git diff abc1234 def5678

# Diff between branches
git diff main feature-branch
git diff main..feature-branch

# Diff specific file
git diff src/main.cpp
git diff HEAD~3 src/main.cpp  # Compare with 3 commits ago

# Show word-level diff
git diff --word-diff

# Show stat only
git diff --stat

# Show names of changed files only
git diff --name-only
git diff --name-status  # With status (A/M/D)

# Show specific commit
git show abc1234
git show HEAD
git show HEAD~2  # 2 commits ago

# Show file at specific commit
git show abc1234:src/main.cpp

# Who changed each line (blame)
git blame src/main.cpp
git blame -L 10,20 src/main.cpp  # Lines 10-20
git blame -e src/main.cpp  # Show email
```

---

### Undoing Changes

```bash
# Discard changes in working directory
git checkout -- src/main.cpp
git restore src/main.cpp  # Modern alternative

# Discard all changes in working directory
git checkout -- .
git restore .

# Unstage file (keep changes)
git reset HEAD src/main.cpp
git restore --staged src/main.cpp

# Unstage all files
git reset HEAD
git restore --staged .

# Undo last commit (keep changes staged)
git reset --soft HEAD~1

# Undo last commit (keep changes unstaged)
git reset HEAD~1
git reset --mixed HEAD~1  # Same thing

# Undo last commit (discard changes) - DANGEROUS
git reset --hard HEAD~1

# Reset to specific commit - DANGEROUS
git reset --hard abc1234

# Reset to remote state - DANGEROUS
git reset --hard origin/main

# Create new commit that undoes a commit
git revert abc1234
git revert HEAD  # Revert last commit

# Revert without committing
git revert --no-commit abc1234

# Revert merge commit
git revert -m 1 merge-commit-hash

# Clean untracked files (dry-run first!)
git clean -n        # Dry-run
git clean -f        # Force delete files
git clean -fd       # Delete files and directories
git clean -fdx      # Also delete ignored files
```

---

### Stashing

```bash
# Stash current changes
git stash

# Stash with message
git stash save "Work in progress on feature"
git stash push -m "WIP feature"  # Modern syntax

# Stash including untracked files
git stash -u
git stash --include-untracked

# Stash including ignored files
git stash -a
git stash --all

# List stashes
git stash list

# Show stash contents
git stash show
git stash show -p              # With diff
git stash show stash@{1}       # Specific stash

# Apply latest stash (keep in stash list)
git stash apply

# Apply specific stash
git stash apply stash@{2}

# Apply and remove from stash list
git stash pop
git stash pop stash@{1}

# Drop specific stash
git stash drop stash@{1}

# Clear all stashes
git stash clear

# Create branch from stash
git stash branch new-branch-name
git stash branch new-branch stash@{1}
```

---

### Tags

```bash
# List tags
git tag
git tag -l
git tag -l "v1.*"  # Pattern match

# Create lightweight tag
git tag v1.0.0

# Create annotated tag (recommended)
git tag -a v1.0.0 -m "Phase 2 complete"

# Tag specific commit
git tag -a v1.0.0 abc1234 -m "Message"

# Show tag info
git show v1.0.0

# Push single tag
git push origin v1.0.0

# Push all tags
git push --tags
git push origin --tags

# Delete local tag
git tag -d v1.0.0

# Delete remote tag
git push origin --delete v1.0.0
git push origin :refs/tags/v1.0.0

# Checkout tag (detached HEAD)
git checkout v1.0.0

# Create branch from tag
git checkout -b hotfix v1.0.0
```

---

### Remote Repositories

```bash
# List remotes
git remote
git remote -v  # With URLs

# Add remote
git remote add origin https://github.com/MarcusLai07/cafe-engine.git
git remote add upstream https://github.com/original/repo.git

# Remove remote
git remote remove origin
git remote rm origin

# Rename remote
git remote rename origin old-origin

# Change remote URL
git remote set-url origin https://github.com/MarcusLai07/new-repo.git

# Show remote details
git remote show origin

# Prune stale remote tracking branches
git remote prune origin

# Fetch from all remotes
git fetch --all

# Update remote refs
git remote update
```

---

### Cherry-Pick

```bash
# Apply specific commit to current branch
git cherry-pick abc1234

# Cherry-pick multiple commits
git cherry-pick abc1234 def5678

# Cherry-pick range of commits
git cherry-pick abc1234..def5678

# Cherry-pick without committing
git cherry-pick --no-commit abc1234
git cherry-pick -n abc1234

# Abort cherry-pick
git cherry-pick --abort

# Continue after resolving conflicts
git cherry-pick --continue
```

---

### Submodules

```bash
# Add submodule
git submodule add https://github.com/user/lib.git libs/lib

# Initialize submodules (after cloning)
git submodule init
git submodule update
# Or combined
git submodule update --init

# Update all submodules to latest
git submodule update --remote

# Clone with submodules
git clone --recursive https://github.com/user/repo.git

# Remove submodule
git submodule deinit libs/lib
git rm libs/lib
```

---

### Debugging & Finding Issues

```bash
# Find which commit introduced a bug (binary search)
git bisect start
git bisect bad                 # Current commit is bad
git bisect good abc1234        # This commit was good
# Git checks out middle commit, you test, then:
git bisect good  # or
git bisect bad
# Repeat until found, then:
git bisect reset

# Automated bisect with test script
git bisect start HEAD abc1234
git bisect run ./test-script.sh

# Find who changed a line
git blame src/main.cpp

# Search for string in all commits
git log -S "search_term"

# Search with regex
git log -G "pattern.*regex"

# Find commits that changed function
git log -L :function_name:src/main.cpp
```

---

### Maintenance & Optimization

```bash
# Garbage collection
git gc

# Aggressive garbage collection
git gc --aggressive

# Check repository integrity
git fsck

# Show object sizes
git count-objects -v

# Prune unreachable objects
git prune

# Repack repository
git repack -a -d
```

---

## Daily Workflow Examples

### Basic Daily Workflow

```bash
# Start of day - get latest changes
git fetch
git pull

# Work on code...

# Check what changed
git status
git diff

# Stage and commit
git add .
git commit -m "feat: Implement feature X"

# Push changes
git push
```

### Feature Branch Workflow

```bash
# Create feature branch
git checkout main
git pull
git checkout -b feature/customer-spawning

# Work on feature...
git add .
git commit -m "feat: Add spawn timer"

# More work...
git add .
git commit -m "feat: Add customer pool"

# Push feature branch
git push -u origin feature/customer-spawning

# When done - merge to main
git checkout main
git pull
git merge feature/customer-spawning
git push

# Clean up
git branch -d feature/customer-spawning
git push origin --delete feature/customer-spawning
```

### Sync with Remote (Multiple Machines)

```bash
# On machine A - push your work
git add .
git commit -m "Progress on machine A"
git push

# On machine B - get the changes
git fetch
git pull

# Work on machine B...
git add .
git commit -m "Progress on machine B"
git push

# Back on machine A
git pull
```

### Fix Mistake in Last Commit

```bash
# Forgot to add a file
git add forgotten-file.cpp
git commit --amend --no-edit

# Wrong commit message
git commit --amend -m "Correct message"

# Already pushed? Force push (only if working alone!)
git push --force-with-lease
```

---

## Commit Message Format

### Standard Format

```
<type>: <short description>

<optional body with details>
```

### Types

| Type | Use For |
|------|---------|
| `feat` | New feature |
| `fix` | Bug fix |
| `docs` | Documentation only |
| `refactor` | Code change that doesn't add feature or fix bug |
| `test` | Adding tests |
| `chore` | Build, config, dependencies |
| `style` | Formatting, no code change |
| `perf` | Performance improvement |

### Examples

```bash
git commit -m "feat: Add customer spawning system"
git commit -m "fix: Correct isometric depth sorting"
git commit -m "docs: Update Phase 3 progress in ROADMAP"
git commit -m "refactor: Extract sprite batching into separate class"
git commit -m "test: Add unit tests for economy system"
git commit -m "chore: Update CMake to 3.25"
```

---

## Troubleshooting

### "Permission denied"

```bash
gh auth status
gh auth login
```

### "Repository not found"

```bash
git remote -v
git remote set-url origin https://github.com/MarcusLai07/cafe-engine.git
```

### "Merge conflicts"

```bash
# See conflicted files
git status

# Edit files to resolve conflicts (look for <<<<<<< markers)

# After resolving
git add .
git commit -m "Resolve merge conflicts"
```

### "Detached HEAD"

```bash
# Create branch to save work
git checkout -b save-my-work

# Or go back to a branch
git checkout main
```

### "Your branch is behind"

```bash
git pull --rebase
# Or
git fetch
git rebase origin/main
```

### Undo Accidental Commit to Wrong Branch

```bash
# Save the commit hash
git log -1  # Copy the hash

# Reset current branch
git reset --hard HEAD~1

# Switch to correct branch and cherry-pick
git checkout correct-branch
git cherry-pick <hash>
```

---

## GitHub CLI Commands

```bash
# Auth
gh auth login
gh auth status
gh auth logout

# Repo
gh repo create
gh repo clone MarcusLai07/cafe-engine
gh repo view
gh repo view --web  # Open in browser

# Pull Requests
gh pr create
gh pr list
gh pr view 123
gh pr checkout 123
gh pr merge 123

# Issues
gh issue create
gh issue list
gh issue view 123

# Gists
gh gist create file.txt
gh gist list
```

---

## Quick Reference Card

```bash
# === SETUP ===
git config user.name "Name"
git config user.email "email"
git clone <url>
git init

# === DAILY ===
git status              # What's changed?
git diff                # See changes
git add .               # Stage all
git commit -m "msg"     # Commit
git push                # Upload
git pull                # Download

# === BRANCHES ===
git branch              # List
git checkout -b name    # Create & switch
git checkout main       # Switch
git merge branch        # Merge
git branch -d name      # Delete

# === HISTORY ===
git log --oneline       # View commits
git show abc123         # View commit
git blame file          # Who changed?

# === UNDO ===
git restore file        # Discard changes
git restore --staged f  # Unstage
git reset --soft HEAD~1 # Undo commit (keep changes)
git reset --hard HEAD~1 # Undo commit (lose changes)
git revert abc123       # Undo with new commit

# === REMOTE ===
git fetch               # Download (no merge)
git pull                # Download + merge
git push                # Upload
git remote -v           # Show remotes

# === STASH ===
git stash               # Save temporarily
git stash pop           # Restore
git stash list          # Show stashes

# === TAGS ===
git tag v1.0.0          # Create
git push --tags         # Push tags
```

---

## End of Phase Checklist

When completing a phase:

1. [ ] All code committed
2. [ ] ROADMAP.md updated with `[x]` for completed tasks
3. [ ] Commit message mentions phase completion
4. [ ] Push to GitHub
5. [ ] Create tag: `git tag -a v0.X.0 -m "Phase X complete"`
6. [ ] Push tag: `git push --tags`
