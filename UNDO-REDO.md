PS: Entries may contain a stroke with undo=false|true only when they are in the beginning of a path. Otherwise undo=false

* When an active stroke is in the middle of the path:
  - Undo: replaces the entry content the stroke before it
  - Redo: replaces the entry content the stroke next to it
* When an active stroke is in the beginning of the path and has undo = false:
  - Undo: marks it as undo=true
  - Redo: replaces the entry content the stroke next to it (if it exists)
* When an active stroke is in the beginning of the path and has undo = true:
  - Undo: does nothing
  - Redo: sets undo=true
* When an active stroke is in the end of the path:
  - Undo: replaces the entry content the stroke before it
  - Redo: does nothing

* Undo
  - if: USQ
    - if: An entry exists
      - if: beginning of path with undo = false
        - Set undo=true
      - if: beginning of path with undo = true
        - Call the SM undo
      - else:
        - Update the entry content to the UCS before it
    - if: An entry doesn't exist
      - Call the SM undo
  - if: SM
    - if: An entry exists
      - beginning of path with undo = false
        - Set undo=true
      - beginning of path with undo = true
        - do nothing

* Redo
  - if: SM
    - if: An entry exists
      - if: beginning of path with undo = true
        - Set undo=false
      - if: beginning of path with undo = false
        - Update the entry content to the stroke next in path (unless it is end of path)
      - if: end of the path
        - call redo on USQ
    - if: An entry doesn't exist
      - call redo on USQ
  
  <!-- Still undone -->
  - if: USQ
    - if: An entry exists
      - beginning of path with undo = false
        - Set undo=true
      - beginning of path with undo = true
        - Call the SM undo
    - else:
      - Update the entry content to the UCS before it
    - if: An entry doesn't exist
      - Call the SM undo
  