import { render, screen, fireEvent } from '@testing-library/react'
import { describe, it, expect, vi } from 'vitest'
import React from 'react'
import LibraryTab from '../LibraryTab'

// Mock dependencies
vi.mock('@tauri-apps/api/core', () => ({
  convertFileSrc: vi.fn((path) => `mocked-src://${path}`),
}))

vi.mock('../utils/platform', () => ({
  getPlatform: vi.fn(() => ({ name: 'Mega Drive' })),
}))

// Mock lucide icons - dynamic mock to avoid "missing export" errors
vi.mock('lucide-react', () => ({
  Play: () => <div data-testid="play-icon" />,
  Trash2: () => <div data-testid="trash-icon" />,
  AlertCircle: () => <div data-testid="alert-icon" />,
  X: () => <div data-testid="x-icon" />,
}))

const mockGames = [
  { id: 1, title: 'Sonic', path: '/sonic.bin', cover: null, playCount: 5, lastPlayed: '2023-01-01' },
  { id: 2, title: 'Mario', path: '/mario.nes', cover: null, playCount: 0 },
]

describe('LibraryTab', () => {
  it('renders library when games are present', () => {
    render(
      <LibraryTab 
        games={mockGames} 
        selectedGame={mockGames[0]} 
        setSelectedGame={vi.fn()}
        onPlay={vi.fn()}
        onDelete={vi.fn()}
        onRename={vi.fn()}
      />
    )
    // Check for title in heading specifically
    expect(screen.getByRole('heading', { name: /Sonic/i })).toBeInTheDocument()
    expect(screen.getByText('MEGA DRIVE')).toBeInTheDocument()
    expect(screen.getByText('YOUR COLLECTION')).toBeInTheDocument()
  })

  it('renders loading state', () => {
    render(<LibraryTab games={[]} isLoading={true} />)
    expect(screen.queryByText('YOUR COLLECTION')).not.toBeInTheDocument()
  })

  it('renders empty state when no games', () => {
    render(<LibraryTab games={[]} isLoading={false} />)
    expect(screen.getByText('Your library is currently empty.')).toBeInTheDocument()
  })

  it('calls setSelectedGame when a game is clicked in carousel', () => {
    const setSelectedGame = vi.fn()
    render(
      <LibraryTab 
        games={mockGames} 
        selectedGame={mockGames[0]} 
        setSelectedGame={setSelectedGame}
      />
    )
    // Target Mario in the carousel track
    const marioCard = screen.getAllByText('Mario').find(el => el.closest('.cover-card'))
    fireEvent.click(marioCard)
    expect(setSelectedGame).toHaveBeenCalledWith(mockGames[1])
  })

  it('opens delete confirmation modal', () => {
    render(
      <LibraryTab 
        games={mockGames} 
        selectedGame={mockGames[0]} 
        onDelete={vi.fn()}
      />
    )
    const deleteBtn = screen.getByTestId('trash-icon').parentElement
    fireEvent.click(deleteBtn)
    expect(screen.getByText('Delete ROM?')).toBeInTheDocument()
    expect(screen.getByTestId('alert-icon')).toBeInTheDocument()
  })

  it('enters rename mode on title click', () => {
    render(
      <LibraryTab 
        games={mockGames} 
        selectedGame={mockGames[0]} 
        onRename={vi.fn()}
      />
    )
    const title = screen.getByRole('heading', { name: /Sonic/i })
    fireEvent.click(title)
    
    const input = screen.getByDisplayValue('Sonic')
    expect(input).toBeInTheDocument()
    
    fireEvent.change(input, { target: { value: 'Sonic 2' } })
    fireEvent.keyDown(input, { key: 'Enter', code: 'Enter' })
  })
})
