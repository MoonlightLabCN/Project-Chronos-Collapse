# Chronos Studio: a deliberately small developer-side desktop UI for packing
# a local Chronos project. It uses the same Build-ChronosProject.ps1 pipeline
# that automated builds use, so there is only one package format to maintain.

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName PresentationFramework
Add-Type -AssemblyName System.Windows.Forms

$studioDirectory = $PSScriptRoot
$builder = Join-Path $studioDirectory 'Build-ChronosProject.ps1'
$sampleDirectory = Join-Path (Split-Path -Path $studioDirectory -Parent) 'Samples\HelloChronos'

[xml]$xaml = @'
<Window xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
        Title="Chronos Studio" Width="720" Height="470" MinWidth="620" MinHeight="390"
        WindowStartupLocation="CenterScreen" Background="#101827" Foreground="#E6EDF7">
  <Grid Margin="24">
    <Grid.RowDefinitions>
      <RowDefinition Height="Auto" />
      <RowDefinition Height="Auto" />
      <RowDefinition Height="Auto" />
      <RowDefinition Height="*" />
      <RowDefinition Height="Auto" />
    </Grid.RowDefinitions>
    <TextBlock Text="Chronos Studio" FontSize="28" FontWeight="SemiBold" />
    <TextBlock Grid.Row="1" Margin="0,6,0,22" Foreground="#B8C4D9" TextWrapping="Wrap"
               Text="选择一个含 chronos.json、C 源码和 XAML 片段的本地项目，生成可由 Chronos Runtime 运行的 .chronos 程序包。" />
    <Grid Grid.Row="2" Margin="0,0,0,12">
      <Grid.ColumnDefinitions><ColumnDefinition Width="*"/><ColumnDefinition Width="116"/></Grid.ColumnDefinitions>
      <TextBox x:Name="ProjectPath" Height="38" Padding="10,7" Background="#172238" Foreground="White"
               BorderBrush="#3A506F" Text="" />
      <Button x:Name="BrowseButton" Grid.Column="1" Margin="10,0,0,0" Content="选择项目" />
    </Grid>
    <TextBox x:Name="LogBox" Grid.Row="3" IsReadOnly="True" TextWrapping="Wrap" VerticalScrollBarVisibility="Auto"
             Background="#0A1120" Foreground="#C8D5E8" BorderBrush="#2A3B55" FontFamily="Consolas" Padding="12" />
    <StackPanel Grid.Row="4" Orientation="Horizontal" HorizontalAlignment="Right" Margin="0,14,0,0">
      <Button x:Name="OpenSampleButton" Content="载入示例" MinWidth="108" Margin="0,0,10,0" />
      <Button x:Name="BuildButton" Content="编译并生成程序包" MinWidth="150" Background="#2563EB" Foreground="White" />
    </StackPanel>
  </Grid>
</Window>
'@

$reader = New-Object System.Xml.XmlNodeReader $xaml
$window = [Windows.Markup.XamlReader]::Load($reader)
$projectPath = $window.FindName('ProjectPath')
$browseButton = $window.FindName('BrowseButton')
$sampleButton = $window.FindName('OpenSampleButton')
$buildButton = $window.FindName('BuildButton')
$logBox = $window.FindName('LogBox')

$writeLog = {
    param([string]$Text)
    $logBox.AppendText($Text + [Environment]::NewLine)
    $logBox.ScrollToEnd()
}

$projectPath.Text = $sampleDirectory
& $writeLog 'Chronos Studio 已就绪。'
& $writeLog '选择项目后，点击“编译并生成程序包”。'

$browseButton.Add_Click({
    $dialog = New-Object System.Windows.Forms.FolderBrowserDialog
    $dialog.Description = '选择 Chronos 项目目录'
    $dialog.SelectedPath = $projectPath.Text
    if ($dialog.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
        $projectPath.Text = $dialog.SelectedPath
    }
})

$sampleButton.Add_Click({
    $projectPath.Text = $sampleDirectory
    & $writeLog ('已载入示例：' + $sampleDirectory)
})

$buildButton.Add_Click({
    $path = $projectPath.Text.Trim()
    if (-not (Test-Path -LiteralPath $path -PathType Container)) {
        [System.Windows.MessageBox]::Show('请选择存在的项目目录。', 'Chronos Studio') | Out-Null
        return
    }
    if (-not (Test-Path -LiteralPath (Join-Path $path 'chronos.json') -PathType Leaf)) {
        [System.Windows.MessageBox]::Show('项目目录中找不到 chronos.json。', 'Chronos Studio') | Out-Null
        return
    }

    $buildButton.IsEnabled = $false
    & $writeLog ('正在编译：' + $path)
    try {
        $result = & $builder -ProjectDirectory $path | Format-List | Out-String
        & $writeLog $result.TrimEnd()
        & $writeLog '完成。把生成的 .chronos 文件发给安装了 Chronos Runtime 的用户即可。'
    }
    catch {
        & $writeLog ('失败：' + $_.Exception.Message)
        [System.Windows.MessageBox]::Show($_.Exception.Message, 'Chronos Studio - 编译失败') | Out-Null
    }
    finally {
        $buildButton.IsEnabled = $true
    }
})

$window.ShowDialog() | Out-Null
