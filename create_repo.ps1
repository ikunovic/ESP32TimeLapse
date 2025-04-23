$token = Read-Host -Prompt 'Enter your GitHub Personal Access Token'

$headers = @{
    Authorization = "token $token"
}

$body = @{
    name = 'ESP32TimeLapse'
    description = 'ESP32 Time Lapse Camera Project'
    private = $false
} | ConvertTo-Json

Write-Host "Creating repository on GitHub..."
Invoke-RestMethod -Uri 'https://api.github.com/user/repos' -Method Post -Headers $headers -Body $body -ContentType 'application/json'

Write-Host "Setting up Git remote..."
git remote add origin https://github.com/ikunovic/ESP32TimeLapse.git
git push -u origin wrover_pins 