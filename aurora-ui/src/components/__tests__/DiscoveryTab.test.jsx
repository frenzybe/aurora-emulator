import { render, screen, fireEvent, waitFor } from '@testing-library/react'
import { describe, it, expect, vi } from 'vitest'
import React from 'react'
import DiscoveryTab from '../DiscoveryTab'

// Mock dependencies
vi.mock('@tauri-apps/api/core', () => ({
  invoke: vi.fn(),
}))

vi.mock('lucide-react', () => ({
  Search: () => <div data-testid="search-icon" />,
  Download: () => <div data-testid="download-icon" />,
  ExternalLink: () => <div data-testid="external-icon" />,
  Box: () => <div data-testid="box-icon" />,
  Gamepad2: () => <div data-testid="gamepad-icon" />,
  Loader2: () => <div data-testid="loader-icon" />,
  CheckCircle2: () => <div data-testid="check-icon" />,
}))

// Mock GameDetailsModal to keep tests simple
vi.mock('../GameDetailsModal', () => ({
  default: ({ onClose }) => <div data-testid="game-details-modal"><button onClick={onClose}>Close</button></div>
}))

const mockResults = [
  { title: 'Super Mario', system: 'nes', url: 'mario-url', meta: 'Platformer', preview: 'mario.png' },
  { title: 'Sonic', system: 'genesis', url: 'sonic-url', meta: 'Speed', preview: 'sonic.png' }
]

describe('DiscoveryTab', () => {
  it('renders search input and title', () => {
    render(<DiscoveryTab showToast={vi.fn()} />)
    expect(screen.getByPlaceholderText('Search for Sonic, Mario, Zelda...')).toBeInTheDocument()
    expect(screen.getByText('Discovery Store')).toBeInTheDocument()
  })

  it('renders search results after search', async () => {
    const { invoke } = await import('@tauri-apps/api/core')
    invoke.mockResolvedValue(mockResults)

    render(<DiscoveryTab showToast={vi.fn()} />)
    
    const input = screen.getByPlaceholderText('Search for Sonic, Mario, Zelda...')
    fireEvent.change(input, { target: { value: 'Mario' } })
    fireEvent.submit(screen.getByText('SEARCH'))

    await waitFor(() => {
      expect(screen.getByText('Super Mario')).toBeInTheDocument()
      expect(screen.getByText('Sonic')).toBeInTheDocument()
    })
  })

  it('calls download_game when INSTALL is clicked', async () => {
    const { invoke } = await import('@tauri-apps/api/core')
    invoke.mockResolvedValueOnce(mockResults) // for initial popular load
    invoke.mockResolvedValueOnce({}) // for download_game
    
    render(<DiscoveryTab showToast={vi.fn()} />)
    
    // Wait for popular games to load (mock initial load)
    await waitFor(() => expect(screen.queryByText('Super Mario')).toBeInTheDocument())
    
    const installBtns = screen.getAllByText('INSTALL')
    fireEvent.click(installBtns[0])
    
    expect(invoke).toHaveBeenCalledWith('download_game', expect.any(Object))
  })

  it('opens details modal when a game card is clicked', async () => {
    const { invoke } = await import('@tauri-apps/api/core')
    invoke.mockResolvedValue(mockResults)

    render(<DiscoveryTab showToast={vi.fn()} />)
    
    await waitFor(() => expect(screen.queryByText('Super Mario')).toBeInTheDocument())
    
    fireEvent.click(screen.getByText('Super Mario'))
    expect(screen.getByTestId('game-details-modal')).toBeInTheDocument()
  })
})
