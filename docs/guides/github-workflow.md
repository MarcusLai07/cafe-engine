# GitHub Workflow Guide

Complete guide for managing the Cafe Engine repository on GitHub.

## Repository Info

- **URL:** https://github.com/MarcusLai07/cafe-engine
- **Account:** MarcusLai07 (personal)
- **Email:** Marcus_Lai@live.com

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

## Daily Workflow

### Check Status

```bash
# See what's changed
git status

# See detailed changes
git diff

# See commit history
git log --oneline -10
```

### Save Your Work (Commit)

```bash
# Stage specific files
git add src/main.cpp src/renderer/

# Or stage everything
git add .

# Commit with message
git commit -m "Phase 3: Add WebGL renderer backend"

# Or multi-line commit
git commit -m "Phase 3.2: WebGL backend implementation

- Created webgl_renderer.cpp
- Implemented sprite batching
- Added shader compilation"
```

### Push to GitHub

```bash
# Push current branch
git push

# If it's a new branch
git push -u origin branch-name
```

### Pull Latest Changes

```bash
# If working from multiple machines
git pull
```

## Branching Strategy

### Recommended: Branch Per Phase

```bash
# Create branch for new phase
git checkout -b phase-3-renderer

# Work on the phase...
git add .
git commit -m "Phase 3.1: Design renderer interface"

# Push branch
git push -u origin phase-3-renderer

# When phase complete, merge to main
git checkout main
git merge phase-3-renderer
git push

# Delete branch (optional)
git branch -d phase-3-renderer
```

### Alternative: Work on Main

For solo projects, working directly on `main` is fine:

```bash
git checkout main
# Work...
git add .
git commit -m "Description"
git push
```

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

### Examples

```bash
# Feature
git commit -m "feat: Add customer spawning system"

# Bug fix
git commit -m "fix: Correct isometric depth sorting"

# Documentation
git commit -m "docs: Update Phase 3 progress in ROADMAP"

# Refactor
git commit -m "refactor: Extract sprite batching into separate class"

# Phase milestone
git commit -m "feat: Complete Phase 3 - Renderer Abstraction

- Metal backend with sprite batching
- WebGL backend for web builds
- Isometric tile rendering
- Cross-platform demo working"
```

## Useful Commands

### Undo Changes

```bash
# Discard changes to a file (not committed)
git checkout -- src/main.cpp

# Unstage a file (keep changes)
git reset HEAD src/main.cpp

# Undo last commit (keep changes)
git reset --soft HEAD~1

# Undo last commit (discard changes) - DANGEROUS
git reset --hard HEAD~1
```

### View History

```bash
# Compact log
git log --oneline

# With graph
git log --oneline --graph

# Show specific commit
git show abc1234

# Show what changed in a file
git log -p src/main.cpp
```

### Stash (Temporarily Save Work)

```bash
# Save current work without committing
git stash

# List stashes
git stash list

# Restore latest stash
git stash pop

# Restore specific stash
git stash apply stash@{1}
```

### Tags (Mark Releases)

```bash
# Create tag
git tag -a v0.1.0 -m "Phase 2 complete"

# Push tags
git push --tags

# List tags
git tag -l
```

## GitHub Desktop Alternative

If you prefer GUI:

1. Open GitHub Desktop
2. File → Add Local Repository
3. Select `/Users/ctg/Documents/GitHub/cafe-engine`
4. Sign into your personal account (MarcusLai07)
5. Use the GUI for commits and pushes

**Note:** If GitHub Desktop is logged into work account:
- Preferences → Accounts → Sign out of work account
- Sign into MarcusLai07
- Or use CLI for this repo, Desktop for work repos

## Multiple GitHub Accounts

### Per-Repo Config (Current Setup)

```bash
# This repo uses personal account
cd /Users/ctg/Documents/GitHub/cafe-engine
git config user.name "MarcusLai07"
git config user.email "Marcus_Lai@live.com"

# Work repos use global config
git config --global user.name "WorkUsername"
git config --global user.email "work@email.com"
```

### Check Current Config

```bash
# Repo-specific
git config user.name
git config user.email

# Global
git config --global user.name
git config --global user.email
```

## Troubleshooting

### "Permission denied"

```bash
# Check which account is being used
gh auth status

# Re-login
gh auth login
```

### "Repository not found"

```bash
# Verify remote URL
git remote -v

# Fix if wrong
git remote set-url origin https://github.com/MarcusLai07/cafe-engine.git
```

### "Merge conflicts"

```bash
# After a failed merge/pull, fix conflicts in files, then:
git add .
git commit -m "Resolve merge conflicts"
```

### Wrong Account Pushing

```bash
# Check repo config
git config user.name
git config user.email

# Fix for this repo
git config user.name "MarcusLai07"
git config user.email "Marcus_Lai@live.com"
```

## Quick Reference

```bash
# Daily workflow
git status                  # Check what's changed
git add .                   # Stage all changes
git commit -m "message"     # Commit
git push                    # Push to GitHub

# Sync with remote
git pull                    # Get latest

# Branching
git checkout -b new-branch  # Create & switch
git checkout main           # Switch to main
git merge branch-name       # Merge branch into current

# Undo
git checkout -- file        # Discard file changes
git reset --soft HEAD~1     # Undo commit, keep changes
```

## End of Phase Checklist

When completing a phase:

1. [ ] All code committed
2. [ ] ROADMAP.md updated with `[x]` for completed tasks
3. [ ] Commit message mentions phase completion
4. [ ] Push to GitHub
5. [ ] Optionally tag the release: `git tag -a v0.X.0 -m "Phase X complete"`
